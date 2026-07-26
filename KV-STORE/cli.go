package main

import (
	"bufio"
	"fmt"
	"os"
	"strings"
)

func runCLI(store *Store) {
	scanner := bufio.NewScanner(os.Stdin)
	fmt.Println("Mini KV Store (commands: SET, GET, DELETE, KEYS, QUIT)")
	fmt.Print("> ")

	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			fmt.Print("> ")
			continue
		}

		parts := strings.Fields(line)
		cmd := strings.ToUpper(parts[0])

		switch cmd {
		case "SET":
			if len(parts) < 3 {
				fmt.Println("usage: SET <key> <value>")
				break
			}
			key := parts[1]
			value := strings.Join(parts[2:], " ")
			if err := store.Set(key, value); err != nil {
				fmt.Printf("error: %v\n", err)
			} else {
				fmt.Println("OK")
			}

		case "GET":
			if len(parts) != 2 {
				fmt.Println("usage: GET <key>")
				break
			}
			value, ok := store.Get(parts[1])
			if !ok {
				fmt.Println("NOT FOUND")
			} else {
				fmt.Println(value)
			}

		case "DELETE":
			if len(parts) != 2 {
				fmt.Println("usage: DELETE <key>")
				break
			}
			deleted, err := store.Delete(parts[1])
			if err != nil {
				fmt.Printf("error: %v\n", err)
			} else if !deleted {
				fmt.Println("NOT FOUND")
			} else {
				fmt.Println("OK")
			}

		case "KEYS":
			keys := store.ListKeys()
			if len(keys) == 0 {
				fmt.Println("(empty)")
			} else {
				fmt.Println(strings.Join(keys, ", "))
			}

		case "QUIT", "EXIT":
			fmt.Println("bye")
			return

		default:
			fmt.Println("unknown command (try SET, GET, DELETE, KEYS, QUIT)")
		}

		fmt.Print("> ")
	}

	if err := scanner.Err(); err != nil {
		fmt.Fprintf(os.Stderr, "input error: %v\n", err)
	}
}
