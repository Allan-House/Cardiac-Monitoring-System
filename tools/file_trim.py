#!/usr/bin/env python3

"""
ECG Binary Data Trimmer
Extracts a time range from binary ECG files.

File format: {int16_t raw_value, int64_t timestamp_us}
"""

import struct
import sys
import argparse
from pathlib import Path

def read_binary_samples(filename):
  """
  Read all samples from binary file.

  Returns:
    list of tuples: [(raw_value, timestamp_us), ...]
  """
  samples = []
  record_size = 10 # 2 bytes (int16) + 8 bytes (int64)
  
  with open(filename, 'rb') as f:
    while True:
      data = f.read(record_size)
      if len(data) < record_size:
        break

      raw_value, timestamp_us = struct.unpack('<hq', data)
      samples.append((raw_value, timestamp_us))
  return samples

def trim_samples(samples, t0_sec, t1_sec):
  """
  Extract samples within time range [t0, t1].

  Args:
    samples: List of (raw_value, timestamp_us) tuples.
    t0_sec: Start time in seconds (relative to first sample).
    t1_sec: End time in seconds (relative to first sample).
  
  Returns:
    list: Filtered samples within time range.
  """
  if not samples:
    return []
  
  first_timestamp = samples[0][1]

  t0_us = int(t0_sec * 1_000_000)
  t1_us = int(t1_sec * 1_000_000)

  trimmed = []
  for raw_value, timestamp_us in samples:
    relative_time = timestamp_us - first_timestamp

    if t0_us <= relative_time <= t1_us:
      trimmed.append((raw_value, timestamp_us))
  
  return trimmed

def write_binary_samples(samples, filename):
  """
  Write samples to binary file.

  Args:
    samples: List of (raw_value, timestamp_us) tuples.
    filename: output file path.
  """
  with open(filename, 'wb') as f:
    for raw_value, timestamp_us in samples:
      data = struct.pack('<hq', raw_value, timestamp_us)
      f.write(data)

def format_time(seconds):
  """Format seconds as MM:SS.mmm"""
  minutes = int(seconds // 60)
  secs = seconds % 60
  return f"{minutes:02d}:{secs:06.3f}"

def main():
  parser = argparse.ArgumentParser(
    description='Trim ECG binary data by time range',
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog="""
      Examples:
        python3 file_trim.py input.bin 0 10 -o output.bin
        python3 file_trim.py input.bin 5.5 15.2 -o trimmed.bin
        python3 file_trim.py input.bin 20 50
    """
  )

  parser.add_argument('input', type=str, help='Input binary file path.')
  parser.add_argument('t0', type=float, help='Start time in seconds.')
  parser.add_argument('t1', type=float, help='End time in seconds.')
  parser.add_argument('-o', '--output', type=str,
                      help='Output file path (default: input_trimmed.bin)')

  args = parser.parse_args()

  if args.t0 < 0:
    print(f"Error: t0 must be >= 0", file=sys.stderr)
    return 1
  
  if args.t1 <= args.t0:
    print(f"Error: t1 must be > t0", file=sys.stderr)
    return 1

  if args.output is None:
    input_path = Path(args.input)
    args.output = str(input_path.parent / f"{input_path.stem}_trimmed.bin")

  if not Path(args.input).exists():
    print(f"Error: Input file not found: {args.input}", file=sys.stderr)
    return 1
  
  print(f"Reading input file: {args.input}")
  
  try:
    samples = read_binary_samples(args.input)
    
    if not samples:
      print("Error: No samples found in input file", file=sys.stderr)
      return 1
    
    print(f"Loaded {len(samples)} samples")
    
    first_ts = samples[0][1]
    last_ts = samples[-1][1]
    duration_sec = (last_ts - first_ts) / 1_000_000
    
    print(f"File duration: {format_time(duration_sec)}")
    print(f"Trimming range: {format_time(args.t0)} to {format_time(args.t1)}")
    
    if args.t1 > duration_sec:
      print(f"Warning: t1 ({args.t1}s) exceeds file duration ({duration_sec:.3f}s)")
      print(f"         Trimming to end of file")
    
    trimmed_samples = trim_samples(samples, args.t0, args.t1)
    
    if not trimmed_samples:
      print("Error: No samples found in specified time range", file=sys.stderr)
      return 1
    
    print(f"Extracted {len(trimmed_samples)} samples")
    
    write_binary_samples(trimmed_samples, args.output)
    
    output_first_ts = trimmed_samples[0][1]
    output_last_ts = trimmed_samples[-1][1]
    output_duration = (output_last_ts - output_first_ts) / 1_000_000
    
    print(f"Output duration: {format_time(output_duration)}")
    print(f"Output file: {args.output}")
    print("Done!")
    
    return 0
      
  except FileNotFoundError:
    print(f"Error: File not found: {args.input}", file=sys.stderr)
    return 1
  except Exception as e:
    print(f"Error: {e}", file=sys.stderr)
    return 1


if __name__ == "__main__":
  sys.exit(main())
