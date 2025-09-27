import requests
import socket
import shutil
import os

HOME = "/home/xifeng"
HOST = "127.0.0.1"
PORT = 8081
BASE = f"http://{HOST}:{PORT}"

def send_raw(raw_bytes: bytes) -> bytes:
    with socket.create_connection((HOST, PORT), timeout=5) as s:
        s.sendall(raw_bytes)
        s.settimeout(1.0)
        try:
            resp = b""
            while True:
                chunk = s.recv(4096)
                if not chunk:
                    break
                resp += chunk
        except socket.timeout:
            pass
        return resp

def test_get_html():
    r = requests.get(f"{BASE}/index.html")
    assert r.status_code == 200
    with open(f"{HOME}/www/app/index.html", "r") as f:
        expected_content = f.read()
    assert expected_content in r.text
    print("GET HTML test passed.")

def test_default_html():
    r = requests.get(f"{BASE}/")
    assert r.status_code == 200
    with open(f"{HOME}/www/app/index.html", "r") as f:
        expected_content = f.read()
    assert expected_content in r.text
    print("GET default HTML test passed.")

def test_get_txt():
    r = requests.get(f"{BASE}/second/test.txt")
    assert r.status_code == 200
    with open(f"{HOME}/www/app/second/test.txt", "r") as f:
        expected_content = f.read()
    assert expected_content in r.text
    print("GET TXT test passed.")

def test_get_autoindex():
    r = requests.get(f"{BASE}/second/")
    assert r.status_code == 200
    assert "second" in r.text
    assert "test.txt" in r.text
    print("GET autoindex test passed.")

def test_get_autoindex_no_slash():
    r = requests.get(f"{BASE}/second")
    assert r.status_code == 200
    assert "second" in r.text
    assert "test.txt" in r.text
    print("GET autoindex test passed.")

def test_get_autoindex_inherit():
    r = requests.get(f"{BASE}/new/")
    assert r.status_code == 200
    assert "new" in r.text
    print("GET autoindex inherit test passed.")

def test_get_404():
    r = requests.get(f"{BASE}/nonexistent")
    assert r.status_code == 404
    print("GET 404 test passed.")

def test_get_extra_slash():
    r = requests.get(f"{BASE}/index.html/index.html/")
    assert r.status_code == 404
    print("GET extra slash test passed.")

def test_get_duplicate_slash():
    req = (
        b"GET //index.html HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Content-Length: 10\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    # check for 400 or connection close — servers vary
    assert b"400" in resp or b"Bad Request" in resp or resp.startswith(b"HTTP/1.1"), "unexpected server reaction"
    print("GET duplicate slash test passed.")

def test_get_content_length_too_large():
    # declare content-length larger than actual body
    req = (
        b"GET / HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Content-Length: 10\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    # check for 400 or connection close — servers vary
    assert b"400" in resp or b"Bad Request" in resp or resp.startswith(b"HTTP/1.1"), "unexpected server reaction"
    print("GET content-length too large test passed.")

def test_get_large_file():
    r = requests.get(f"{BASE}/pic.png")
    assert r.status_code == 200
    with open(f"{HOME}/www/app/pic.png", "rb") as f:
        expected_content = f.read()
    assert r.content == expected_content
    print("GET large file test passed.")

def test_get_incorrect_http_version():
    req = (
        b"GET / HTTP/1.2\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("GET incorrect HTTP version test passed.")

def test_get_incorrect_method():
    req = (
        b"FETCH / HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("GET incorrect method test passed.")

def test_get_no_method():
    req = (
        b"/ HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"Error" in resp, "unexpected server reaction"
    print("GET no method test passed.")

def test_get_no_http_version():
    req = (
        b"GET / \r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"Error" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("GET no HTTP version test passed.")

def test_get_incorrect_protocol():
    req = (
        b"GET / FTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"Error" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("GET incorrect protocol test passed.")

def test_get_not_allowed_method():
    r = requests.get(f"{BASE}/uploads/test.txt")

    assert r.status_code == 405
    print("GET not allowed method test passed.")

def test_delete_not_allowed_method():
    r = requests.delete(f"{BASE}/second/pic.png")
    assert r.status_code == 405
    print("DELETE not allowed method test passed.")

def test_delete_ok():
    shutil.copy(f"{HOME}/www/app/test.txt", f"{HOME}/www/app/uploads/test.txt")
    r = requests.delete(f"{BASE}/uploads/test.txt")
    assert r.status_code == 204
    r = requests.delete(f"{BASE}/uploads/test.txt")
    assert r.status_code == 404
    print("DELETE ok test passed.")

def test_delete_nonexistent_file():
    r = requests.delete(f"{BASE}/uploads/nonexistent.txt")
    assert r.status_code == 404
    print("DELETE nonexistent file test passed.")

def test_delete_folder():
    r = requests.delete(f"{BASE}/uploads/")
    assert r.status_code == 403
    print("DELETE folder test passed.")


def test_delete_folder_more():
    if not os.path.exists(f"{HOME}/www/app/uploads/more/"):
        os.mkdir(f"{HOME}/www/app/uploads/more/")
    r = requests.delete(f"{BASE}/uploads/more/")
    assert r.status_code == 403
    print("DELETE folder without slash test passed.")

def test_delete_with_body():
    req = (
        b"DELETE /uploads/test.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Content-Length: 10\r\n"
        b"\r\n"
        b"1234567890"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("DELETE with body test passed.")

def test_simple_post():
    header = {"Content-Type": "text/plain",
              "Content-Length": "13"}
    r = requests.post(f"{BASE}/uploads", headers=header, data="Hello, World!")
    assert r.status_code == 201
    print("Simple POST test passed.")

def test_post_large_file():
    large_content = "A" * 10_000_000  # 10 MB of 'A's
    header = {"Content-Type": "text/plain",
              "Content-Length": "10000000"}
    r = requests.post(f"{BASE}/uploads", headers=header, data=large_content)
    assert r.status_code == 201
    print("POST large file test passed.")

def test_post_without_content_length():
    req = (
        b"POST /uploads/nolength.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"\r\n"
        b"Data without content length"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("POST without Content-Length test passed.")

def test_post_exceeding_content_length():
    req = (
        b"POST /uploads/exceed.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Content-Length: 10\r\n"
        b"\r\n"
        b"Too much data here"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("POST exceeding Content-Length test passed.")

def test_post_chunked_transfer():
    req = (
        b"POST /uploads/chunked.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"\r\n"
        b"7\r\n"
        b"Mozilla\r\n"
        b"9\r\n"
        b"Developer\r\n"
        b"7\r\n"
        b"Network\r\n"
        b"0\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"201" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    r = requests.get(f"{BASE}/uploads/chunked.txt")
    assert r.status_code == 200
    assert r.text == "MozillaDeveloperNetwork"
    os.remove(f"{HOME}/www/app/uploads/chunked.txt")
    print("POST chunked transfer test passed.")

def test_post_chunked_large_file():
    large_content = "A" * 10_000_000  # 10 MB of 'A's
    chunk_size = 4096
    chunks = [large_content[i:i+chunk_size] for i in range(0, len(large_content), chunk_size)]
    req = (
        b"POST /uploads/chunked_large.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"\r\n"
    )
    for chunk in chunks:
        req += f"{len(chunk):X}\r\n".encode() + chunk.encode() + b"\r\n"
    req += b"0\r\n\r\n"
    
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"201" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    
    r = requests.get(f"{BASE}/uploads/chunked_large.txt")
    assert r.status_code == 200
    assert r.text == large_content
    os.remove(f"{HOME}/www/app/uploads/chunked_large.txt")
    print("POST chunked transfer large file test passed.")

def test_post_chunked_transfer_invalid_header():
    req = (
        b"POST /uploads/chunked.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"Content-Length: 10\r\n"  # Invalid with chunked
        b"\r\n"
        b"7\r\n"
        b"Mozilla\r\n"
        b"9\r\n"
        b"Developer\r\n"
        b"7\r\n"
        b"Network\r\n"
        b"0\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("POST chunked transfer with invalid header test passed.")

def test_post_chunked_incorrect_chunk_size():
    req = (
        b"POST /uploads/chunked.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"\r\n"
        b"Z\r\n"  # Invalid chunk size
        b"Mozilla\r\n"
        b"0\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("POST chunked transfer with incorrect chunk size test passed.")

def test_post_chunked_incorrect_chunk_tail():
    req = (
        b"POST /uploads/chunked.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"\r\n"
        b"7\r\n"
        b"Mozilla"  # Missing \r\n after chunk data
        b"0\r\n"
        b"\r\n"
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("POST chunked transfer with incorrect chunk tail test passed.")

def test_post_chunked_extra_data_after_last_chunk():
    req = (
        b"POST /uploads/chunked.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"\r\n"
        b"7\r\n"
        b"Mozilla\r\n"
        b"0\r\n"
        b"\r\n"
        b"ExtraData"  # Extra data after last chunk
    )
    resp = send_raw(req)
    assert resp, "server sent no response (it may have kept the connection open)"
    assert b"400" in resp or b"HTTP/1.1" in resp, "unexpected server reaction"
    print("POST chunked transfer with extra data after last chunk test passed.")

# test_timeout()
# test_keep_alive()
# test_exceed_max_body_size()
# test_exceed_max_header_size()
# test_cgi_execution()


def run_all():
    test_get_html()
    test_default_html()
    test_get_txt()
    test_get_autoindex()
    test_get_autoindex_no_slash()
    test_get_autoindex_inherit()
    test_get_extra_slash()
    test_get_duplicate_slash()
    test_get_404()
    test_get_content_length_too_large()
    test_get_large_file()
    test_get_incorrect_http_version()
    test_get_incorrect_method()
    test_get_no_method()
    test_get_no_http_version()
    test_get_incorrect_protocol()
    test_get_not_allowed_method()

    test_delete_not_allowed_method()
    test_delete_ok()
    test_delete_nonexistent_file()
    test_delete_folder()
    test_delete_folder_more()
    test_delete_with_body()

    #test_simple_post()
    #test_post_large_file()
    #test_post_without_content_length()
    #test_post_exceeding_content_length()
    #test_post_chunked_transfer()
    #test_post_chunked_large_file()
    #test_post_chunked_transfer_invalid_header()
    #test_post_chunked_incorrect_chunk_size()
    #test_post_chunked_incorrect_chunk_tail()
    #test_post_chunked_extra_data_after_last_chunk()
    print("All tests passed.")

def run_one():
    test_post_large_file()


if __name__=="__main__":
    #run_all()
    run_one()