#!/usr/bin/env python3
import os
import sys

MAX_CONTENT_LENGTH = 1000 * 1024 * 1024

class CGIError(Exception):
    error_map = {
        400: "Bad Request",
        404: "Not Found",
        405: "Method Not Allowed",
        411: "Length Required",
        413: "Payload Too Large",
        415: "Unsupported Media Type",
        500: "Internal Server Error",
        505: "HTTP Version Not Supported",
    }

    def __init__(self, code, message):
        super().__init__(message)
        self.message = message
        self.code = code
        self.description = self.error_map.get(code, "Unknown Error")

class Response:
    def __init__(self, message, status=200, status_message="OK", error=None):
        self.status = error.code if error else status
        self.status_message = error.description if error else status_message
        self.message = message if error is None else error.message

    def write(self):
        headers = (
            f"Status: {self.status} {self.status_message}\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n"
            f"Content-Length: {len(self.message.encode('utf-8'))}\r\n"
            "\r\n"
        )
        sys.stdout.write(headers + self.message)
        sys.stdout.flush()

class Header:
    def __init__(self):
        self.method = os.environ.get("REQUEST_METHOD", "").upper()
        self.chunked = os.environ.get("HTTP_TRANSFER_ENCODING", "").lower() == "chunked"

        if self.method in ("GET", "DELETE"):
            self.content_length = 0
        elif self.chunked:
            self.content_length = MAX_CONTENT_LENGTH + 1
        else:
            try:
                self.content_length = int(os.environ.get("CONTENT_LENGTH") or 0)
            except ValueError:
                raise CGIError(400, "Bad Request: Invalid Content Length")

        self.content_type = os.environ.get("CONTENT_TYPE", "")
        if self.method not in ("GET", "POST"):
            raise CGIError(405, "Method Not Allowed")
        if self.content_length < 0:
            raise CGIError(400, "Bad Request: Invalid Content Length")
        if self.content_length > MAX_CONTENT_LENGTH:
            raise CGIError(413, "Payload Too Large")
        if self.content_type and not self.content_type.startswith("text/plain"):
            raise CGIError(415, "Unsupported Media Type")

class Request:
    def __init__(self):
        self.headers = Header()
        self.body = self._get_body()

    def _get_body(self):
        if self.headers.method == "POST" and self.headers.content_length > 0:
            received = 0
            last_chunk = ""
            while received < self.headers.content_length:
                chunk = sys.stdin.read(
                    min(4096, self.headers.content_length - received)
                )
                if not chunk:
                    break
                last_chunk = chunk  # keep only last
                received += len(chunk)
            if not self.headers.chunked and received != self.headers.content_length:
                raise CGIError(400, "Bad Request: Incomplete request body")
            return f"Bytes received: {received}\nLast chunk: {last_chunk}"
        return ""

    def _handle_get(self):
        return Response("Welcome to the Python CGI application!")

    def _handle_post(self):
        if self.headers.content_length == 0:
            raise CGIError(411, "Length Required")
        return Response(self.body, status=201)

    def dispatch(self):
        if self.headers.method == "GET":
            return self._handle_get()
        elif self.headers.method == "POST":
            return self._handle_post()
        else:
            raise CGIError(405, "Method Not Allowed")

def main():
    try:
        request = Request()
        response = request.dispatch()
        response.write()
    except CGIError as e:
        Response(f"Error: {e.message}", error=e).write()
    except Exception as e:
        with open("python_error.log", "w") as err:
            err.write(f"Error: {str(e)}\n")
        Response(f"Error: {str(e)}", error=CGIError(500, "Internal Server Error")).write()

if __name__ == "__main__":
    main()
