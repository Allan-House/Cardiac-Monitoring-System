import os
import struct
import numpy as np
import matplotlib.pyplot as plt
import argparse

def read_binary_file(filename):
  if not os.path.isfile(filename):
    raise FileNotFoundError(f"The file '{filename}' was not found.")

  with open(filename, 'rb') as f:
    data = f.read()
  
  record_size = 10
  if len(data) % record_size != 0:
    raise ValueError(f"File size is not a multiple of {record_size} bytes.")

  num_records = len(data) // record_size
  voltages_raw = np.empty(num_records, dtype=np.int16)
  timestamps = np.empty(num_records, dtype=np.int64)

  for i in range(num_records):
    offset = i * record_size
    raw = data[offset:offset + record_size]

    (voltage,) = struct.unpack('<h', raw[0:2])
    (timestamp,) = struct.unpack('<q', raw[2:10])

    voltages_raw[i] = voltage
    timestamps[i] = timestamp

  voltages = voltages_raw.astype(np.float32) * 4.096 / 32768.0

  return voltages, timestamps


def plot_ecg(voltages, timestamps, filename):
  # converter timestamps para segundos relativos ao primeiro
  time_sec = (timestamps - timestamps[0]) / 1e6

  fig, ax = plt.subplots(figsize=(15, 6), dpi=100)

  ax.plot(time_sec, voltages, label="ECG Signal", linewidth=1)

  ax.axhline(0, color="black", linestyle='-')
  ax.set_xlabel("Time (s)")
  ax.set_ylabel("Voltage (V)")
  ax.set_title(f'Data from {os.path.basename(filename)}')
  ax.legend()
  plt.show()


def main():
  parser = argparse.ArgumentParser(description="Plot ECG data from binary file")
  parser.add_argument("filename", help="Caminho para o arquivo binário de entrada")
  args = parser.parse_args()

  voltages, timestamps = read_binary_file(args.filename)
  plot_ecg(voltages, timestamps, args.filename)


if __name__ == "__main__":
  main()
