package main

import (
	"bufio"
	"fmt"
	"os"
	"strings"
	"sync"
)

const (
	logFileName          = "data.log"
	compactAfterWrites   = 50
	compactAfterBytes    = 64 * 1024
)

type Store struct {
	mu         sync.RWMutex
	data       map[string]string
	logPath    string
	writeCount int
}

func NewStore(logPath string) *Store {
	if logPath == "" {
		logPath = logFileName
	}
	return &Store{
		data:    make(map[string]string),
		logPath: logPath,
	}
}

func (s *Store) add(key, value string) {
	s.data[key] = value
}

func (s *Store) read(key string) (string, bool) {
	value, ok := s.data[key]
	return value, ok
}

func (s *Store) Set(key, value string) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	s.add(key, value)

	line := serializeSet(key, value)
	if err := appendToLog(s.logPath, line); err != nil {
		return err
	}

	s.writeCount++
	return s.maybeCompact()
}

func (s *Store) Get(key string) (string, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.read(key)
}

func (s *Store) Delete(key string) (bool, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	if _, ok := s.data[key]; !ok {
		return false, nil
	}

	delete(s.data, key)

	line := serializeDelete(key)
	if err := appendToLog(s.logPath, line); err != nil {
		return true, err
	}

	s.writeCount++
	if err := s.maybeCompact(); err != nil {
		return true, err
	}
	return true, nil
}

func (s *Store) ListKeys() []string {
	s.mu.RLock()
	defer s.mu.RUnlock()

	keys := make([]string, 0, len(s.data))
	for key := range s.data {
		keys = append(keys, key)
	}
	return keys
}

func (s *Store) Replay() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	lines, err := readLogLines(s.logPath)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}

	if len(lines) == 0 {
		return nil
	}

	for i, line := range lines {
		isLast := i == len(lines)-1
		op, key, value, ok := parseLogLine(line)
		if !ok {
			if isLast {
				continue
			}
			return fmt.Errorf("malformed log line %d: %q", i+1, line)
		}

		switch op {
		case "SET":
			s.add(key, value)
		case "DELETE":
			delete(s.data, key)
		}
	}

	return nil
}

func (s *Store) maybeCompact() error {
	info, err := os.Stat(s.logPath)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}

	if s.writeCount < compactAfterWrites && info.Size() < compactAfterBytes {
		return nil
	}

	if err := compactLog(s.logPath, s.data); err != nil {
		return err
	}

	s.writeCount = 0
	return nil
}

func serializeSet(key, value string) string {
	return fmt.Sprintf("SET %s %s", key, value)
}

func serializeDelete(key string) string {
	return fmt.Sprintf("DELETE %s", key)
}

func parseLogLine(line string) (op, key, value string, ok bool) {
	line = strings.TrimSpace(line)
	if line == "" {
		return "", "", "", false
	}

	fields := strings.Fields(line)
	if len(fields) < 2 {
		return "", "", "", false
	}

	op = strings.ToUpper(fields[0])
	key = fields[1]

	switch op {
	case "SET":
		if len(fields) < 3 {
			return "", "", "", false
		}
		value = strings.Join(fields[2:], " ")
		return op, key, value, true
	case "DELETE":
		if len(fields) != 2 {
			return "", "", "", false
		}
		return op, key, "", true
	default:
		return "", "", "", false
	}
}

func appendToLog(path, line string) error {
	f, err := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return err
	}
	defer f.Close()

	if _, err := fmt.Fprintln(f, line); err != nil {
		return err
	}

	return f.Sync()
}

func readLogLines(path string) ([]string, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	var lines []string
	scanner := bufio.NewScanner(f)
	for scanner.Scan() {
		line := scanner.Text()
		if strings.TrimSpace(line) != "" {
			lines = append(lines, line)
		}
	}
	if err := scanner.Err(); err != nil {
		return nil, err
	}
	return lines, nil
}

func compactLog(path string, data map[string]string) error {
	tmpPath := path + ".tmp"

	f, err := os.OpenFile(tmpPath, os.O_CREATE|os.O_TRUNC|os.O_WRONLY, 0644)
	if err != nil {
		return err
	}

	for key, value := range data {
		if _, err := fmt.Fprintln(f, serializeSet(key, value)); err != nil {
			f.Close()
			os.Remove(tmpPath)
			return err
		}
	}

	if err := f.Sync(); err != nil {
		f.Close()
		os.Remove(tmpPath)
		return err
	}
	if err := f.Close(); err != nil {
		os.Remove(tmpPath)
		return err
	}

	return os.Rename(tmpPath, path)
}
