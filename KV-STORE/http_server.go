package main

import (
	"encoding/json"
	"fmt"
	"net/http"
)

func startHTTPServer(store *Store, addr string) error {
	mux := http.NewServeMux()

	mux.HandleFunc("/set", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet && r.Method != http.MethodPost {
			writeJSON(w, http.StatusMethodNotAllowed, map[string]string{"error": "method not allowed"})
			return
		}

		key := r.URL.Query().Get("key")
		value := r.URL.Query().Get("value")
		if key == "" || value == "" {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": "key and value are required"})
			return
		}

		if err := store.Set(key, value); err != nil {
			writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error()})
			return
		}

		writeJSON(w, http.StatusOK, map[string]string{"status": "OK", "key": key, "value": value})
	})

	mux.HandleFunc("/get", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			writeJSON(w, http.StatusMethodNotAllowed, map[string]string{"error": "method not allowed"})
			return
		}

		key := r.URL.Query().Get("key")
		if key == "" {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": "key is required"})
			return
		}

		value, ok := store.Get(key)
		if !ok {
			writeJSON(w, http.StatusNotFound, map[string]string{"error": "not found", "key": key})
			return
		}

		writeJSON(w, http.StatusOK, map[string]string{"key": key, "value": value})
	})

	mux.HandleFunc("/delete", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet && r.Method != http.MethodDelete {
			writeJSON(w, http.StatusMethodNotAllowed, map[string]string{"error": "method not allowed"})
			return
		}

		key := r.URL.Query().Get("key")
		if key == "" {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": "key is required"})
			return
		}

		deleted, err := store.Delete(key)
		if err != nil {
			writeJSON(w, http.StatusInternalServerError, map[string]string{"error": err.Error()})
			return
		}
		if !deleted {
			writeJSON(w, http.StatusNotFound, map[string]string{"error": "not found", "key": key})
			return
		}

		writeJSON(w, http.StatusOK, map[string]string{"status": "OK", "key": key})
	})

	mux.HandleFunc("/keys", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			writeJSON(w, http.StatusMethodNotAllowed, map[string]string{"error": "method not allowed"})
			return
		}

		writeJSON(w, http.StatusOK, map[string]any{"keys": store.ListKeys()})
	})

	fmt.Printf("HTTP server listening on %s\n", addr)
	fmt.Println("Routes: GET /set?key=&value=  GET /get?key=  GET /delete?key=  GET /keys")
	return http.ListenAndServe(addr, mux)
}

func writeJSON(w http.ResponseWriter, status int, payload any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(payload)
}
