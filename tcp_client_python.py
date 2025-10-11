#!/usr/bin/env python3
"""
TCP Client for Cardiac Monitoring System
Connects to the embedded system and receives ECG data files.
"""

import socket
import sys
import os
import argparse
from pathlib import Path


class CardiacTCPClient:
  def __init__(self, host, port=8080, output_dir="received_data"):
    self.host = host
    self.port = port
    self.output_dir = Path(output_dir)
    self.socket = None
    
    # Create output directory if it doesn't exist
    self.output_dir.mkdir(parents=True, exist_ok=True)
  
  def connect(self):
    """Connect to the server"""
    try:
      self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      print(f"Connecting to {self.host}:{self.port}...")
      self.socket.connect((self.host, self.port))
      print(f"✓ Connected to {self.host}:{self.port}")
      return True
    except ConnectionRefusedError:
      print(f"✗ Connection refused. Is the server running on {self.host}:{self.port}?")
      return False
    except socket.timeout:
      print(f"✗ Connection timeout")
      return False
    except Exception as e:
      print(f"✗ Connection error: {e}")
      return False
  
  def receive_files(self):
    """Receive files from the server"""
    try:
      # Read file count header: "FILES <count>\n"
      header = self._receive_line()
      
      if not header:
        print("✗ No response from server")
        return False
      
      if header.startswith("ERROR"):
        print(f"✗ Server error: {header}")
        return False
      
      # Parse file count
      parts = header.split()
      if len(parts) != 2 or parts[0] != "FILES":
        print(f"✗ Invalid header format: {header}")
        return False
      
      file_count = int(parts[1])
      print(f"\n Server will send {file_count} file(s)")
      
      # Receive each file
      for i in range(file_count):
        if not self._receive_file(i + 1, file_count):
          return False
      
      print(f"\n✓ Successfully received all {file_count} files!")
      return True
        
    except Exception as e:
      print(f"\n✗ Error receiving files: {e}")
      return False
  
  def _receive_file(self, file_num, total_files):
    """Receive a single file"""
    try:
      # Read file metadata: "FILE <filename> <size>\n"
      metadata = self._receive_line()
      
      if not metadata:
        print(f"✗ No metadata for file {file_num}")
        return False
      
      if metadata.startswith("ERROR"):
        print(f"✗ Server error: {metadata}")
        return False
      
      # Parse metadata
      parts = metadata.split()
      if len(parts) != 3 or parts[0] != "FILE":
        print(f"✗ Invalid metadata format: {metadata}")
        return False
      
      filename = parts[1]
      file_size = int(parts[2])
      
      print(f"\n[{file_num}/{total_files}] Receiving: {filename} ({self._format_size(file_size)})")
      
      # Receive file data
      filepath = self.output_dir / filename
      bytes_received = 0
      
      with open(filepath, 'wb') as f:
        while bytes_received < file_size:
          remaining = file_size - bytes_received
          chunk_size = min(8192, remaining)
          
          chunk = self.socket.recv(chunk_size)
          if not chunk:
            print(f"\n✗ Connection closed unexpectedly")
            return False
          
          f.write(chunk)
          bytes_received += len(chunk)
          
          # Progress indicator
          progress = (bytes_received / file_size) * 100
          print(f"\r  Progress: {progress:.1f}% ({self._format_size(bytes_received)}/{self._format_size(file_size)})", 
                end='', flush=True)
      
      print(f"\n  ✓ Saved to: {filepath}")
      return True
        
    except Exception as e:
      print(f"\n✗ Error receiving file: {e}")
      return False
  
  def _receive_line(self):
    """Receive a line of text (terminated by \n)"""
    line = b''
    while True:
      char = self.socket.recv(1)
      if not char:
        return None
      if char == b'\n':
        break
      line += char
    return line.decode('utf-8').strip()
  
  def _format_size(self, bytes_size):
    """Format bytes to human-readable size"""
    for unit in ['B', 'KB', 'MB', 'GB']:
      if bytes_size < 1024.0:
        return f"{bytes_size:.1f} {unit}"
      bytes_size /= 1024.0
    return f"{bytes_size:.1f} TB"
  
  def close(self):
    """Close the connection"""
    if self.socket:
      self.socket.close()
      print("\nConnection closed")


def main():
  parser = argparse.ArgumentParser(
    description='TCP Client for Cardiac Monitoring System',
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog="""
Examples:
  python3 tcp_client.py 192.168.1.100
  python3 tcp_client.py 192.168.1.100 -p 8080
  python3 tcp_client.py 192.168.1.100 -o my_data
        """
  )
    
  parser.add_argument('host', help='Server IP address (e.g., 192.168.1.100)')
  parser.add_argument('-p', '--port', type=int, default=8080, 
                      help='Server port (default: 8080)')
  parser.add_argument('-o', '--output', default='received_data',
                      help='Output directory for received files (default: received_data)')
  parser.add_argument('-t', '--timeout', type=int, default=60,
                      help='Connection timeout in seconds (default: 60)')
  
  args = parser.parse_args()
  
  print("=" * 60)
  print("Cardiac Monitoring System - TCP Client")
  print("=" * 60)
  
  # Create client
  client = CardiacTCPClient(args.host, args.port, args.output)
  
  # Set socket timeout
  socket.setdefaulttimeout(args.timeout)
  
  try:
    # Connect to server
    if not client.connect():
      return 1
    
    # Receive files
    if not client.receive_files():
      return 1
    
    print("\n" + "=" * 60)
    print("Transfer complete!")
    print("=" * 60)
    return 0
      
  except KeyboardInterrupt:
    print("\n\n✗ Interrupted by user")
    return 1
  except Exception as e:
    print(f"\n✗ Unexpected error: {e}")
    return 1
  finally:
    client.close()


if __name__ == "__main__":
  sys.exit(main())
