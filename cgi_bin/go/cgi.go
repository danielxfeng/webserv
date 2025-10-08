#!/usr/bin/env go run
package main

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"strconv"
)

const MaxContentLength = 1000 * 1024 * 1024

var errorMap = map[int]string{
	400: "Bad Request",
	405: "Method Not Allowed",
	411: "Length Required",
	413: "Payload Too Large",
	415: "Unsupported Media Type",
	500: "Internal Server Error",
}

// writeResponse outputs headers + body
func writeResponse(status int, message string) {
	desc, ok := errorMap[status]
	if !ok {
		switch status {
		case 200:
			desc = "OK"
		case 201:
			desc = "Created"
		default:
			desc = "Unknown"
		}
	}

	body := []byte(message)
	fmt.Fprintf(os.Stdout, "Status: %d %s\r\n", status, desc)
	fmt.Fprintf(os.Stdout, "Content-Type: text/plain; charset=utf-8\r\n")
	fmt.Fprintf(os.Stdout, "Content-Length: %d\r\n", len(body))
	fmt.Fprintf(os.Stdout, "\r\n")
	fmt.Fprint(os.Stdout, message)
	os.Stdout.Sync()
}

// readBody reads stdin up to contentLength and keeps the last chunk
func readBody(contentLength int) (string, error) {
	reader := bufio.NewReader(os.Stdin)
	received := 0
	lastChunk := ""
	for received < contentLength {
		toRead := 4096
		if contentLength-received < toRead {
			toRead = contentLength - received
		}
		buf := make([]byte, toRead)
		n, err := io.ReadFull(reader, buf)
		if err != nil && err != io.ErrUnexpectedEOF {
			return "", fmt.Errorf("read error: %v", err)
		}
		if n == 0 {
			break
		}
		lastChunk = string(buf[:n])
		received += n
	}
	if received != contentLength {
		return "", fmt.Errorf("incomplete body: got %d, expected %d", received, contentLength)
	}
	return fmt.Sprintf("Bytes received: %d\nLast chunk: %s", received, lastChunk), nil
}

func main() {
	method := os.Getenv("REQUEST_METHOD")
	contentLengthStr := os.Getenv("CONTENT_LENGTH")

	var contentLength int
	if contentLengthStr != "" {
		n, err := strconv.Atoi(contentLengthStr)
		if err != nil {
			writeResponse(400, "Bad Request: Invalid Content Length")
			return
		}
		contentLength = n
	}

	switch method {
	case "GET":
		writeResponse(200, "Welcome to the Go CGI application!")
	case "POST":
		if contentLength <= 0 {
			writeResponse(411, "Length Required")
			return
		}
		if contentLength > MaxContentLength {
			writeResponse(413, "Payload Too Large")
			return
		}
		body, err := readBody(contentLength)
		if err != nil {
			writeResponse(400, fmt.Sprintf("Bad Request: %v", err))
			return
		}
		writeResponse(201, body)
	default:
		writeResponse(405, "Method Not Allowed")
	}
}
