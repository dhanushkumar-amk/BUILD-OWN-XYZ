package main

import (
	"flag"
	"fmt"
	"os"
)

func main() {
	mode := flag.String("mode", "http", "run mode: http or cli")
	addr := flag.String("addr", ":8081", "HTTP listen address")
	logPath := flag.String("log", "data.log", "path to the write-ahead log file")
	flag.Parse()

	store := NewStore(*logPath)

	if err := store.Replay(); err != nil {
		fmt.Fprintf(os.Stderr, "replay failed: %v\n", err)
		os.Exit(1)
	}

	switch *mode {
	case "cli":
		runCLI(store)
	case "http":
		if err := startHTTPServer(store, *addr); err != nil {
			fmt.Fprintf(os.Stderr, "server error: %v\n", err)
			os.Exit(1)
		}
	default:
		fmt.Fprintf(os.Stderr, "unknown mode %q (use http or cli)\n", *mode)
		os.Exit(1)
	}
}
