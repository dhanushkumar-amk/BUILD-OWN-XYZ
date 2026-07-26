package main

import (
	"os"
	"path/filepath"
	"testing"
)

func TestStoreBasicOperations(t *testing.T) {
	dir := t.TempDir()
	logPath := filepath.Join(dir, "test.log")
	store := NewStore(logPath)

	if err := store.Set("name", "kiran"); err != nil {
		t.Fatalf("set failed: %v", err)
	}

	value, ok := store.Get("name")
	if !ok || value != "kiran" {
		t.Fatalf("get name = %q, %v; want kiran, true", value, ok)
	}

	if _, ok := store.Get("missing"); ok {
		t.Fatal("expected missing key to return not found")
	}

	deleted, err := store.Delete("name")
	if err != nil || !deleted {
		t.Fatalf("delete failed: deleted=%v err=%v", deleted, err)
	}

	if _, ok := store.Get("name"); ok {
		t.Fatal("key should be gone after delete")
	}
}

func TestReplayAndDurability(t *testing.T) {
	dir := t.TempDir()
	logPath := filepath.Join(dir, "test.log")

	store := NewStore(logPath)
	if err := store.Set("a", "1"); err != nil {
		t.Fatalf("set a failed: %v", err)
	}
	if err := store.Set("b", "2"); err != nil {
		t.Fatalf("set b failed: %v", err)
	}

	restarted := NewStore(logPath)
	if err := restarted.Replay(); err != nil {
		t.Fatalf("replay failed: %v", err)
	}

	value, ok := restarted.Get("a")
	if !ok || value != "1" {
		t.Fatalf("replay a = %q, %v", value, ok)
	}

	value, ok = restarted.Get("b")
	if !ok || value != "2" {
		t.Fatalf("replay b = %q, %v", value, ok)
	}
}

func TestSkipCorruptedLastLine(t *testing.T) {
	dir := t.TempDir()
	logPath := filepath.Join(dir, "test.log")

	if err := os.WriteFile(logPath, []byte("SET good value\nSET bad\n"), 0644); err != nil {
		t.Fatalf("write log: %v", err)
	}

	store := NewStore(logPath)
	if err := store.Replay(); err != nil {
		t.Fatalf("replay failed: %v", err)
	}

	value, ok := store.Get("good")
	if !ok || value != "value" {
		t.Fatalf("good = %q, %v", value, ok)
	}
}

func TestCompaction(t *testing.T) {
	dir := t.TempDir()
	logPath := filepath.Join(dir, "test.log")

	store := NewStore(logPath)
	if err := store.Set("name", "first"); err != nil {
		t.Fatalf("set first: %v", err)
	}
	if err := store.Set("name", "second"); err != nil {
		t.Fatalf("set second: %v", err)
	}

	if err := compactLog(logPath, map[string]string{"name": "second"}); err != nil {
		t.Fatalf("compact: %v", err)
	}

	restarted := NewStore(logPath)
	if err := restarted.Replay(); err != nil {
		t.Fatalf("replay: %v", err)
	}

	value, ok := restarted.Get("name")
	if !ok || value != "second" {
		t.Fatalf("name = %q, %v", value, ok)
	}
}

func TestParseLogLine(t *testing.T) {
	op, key, value, ok := parseLogLine("SET name kiran patel")
	if !ok || op != "SET" || key != "name" || value != "kiran patel" {
		t.Fatalf("parse SET with spaces failed: %q %q %q %v", op, key, value, ok)
	}

	op, key, _, ok = parseLogLine("DELETE name")
	if !ok || op != "DELETE" || key != "name" {
		t.Fatalf("parse DELETE failed: %q %q %v", op, key, ok)
	}

	if _, _, _, ok := parseLogLine("GARBAGE"); ok {
		t.Fatal("expected garbage line to fail parse")
	}
}
