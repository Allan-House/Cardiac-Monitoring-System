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

  return voltages, timestamps, None


def read_csv_file(filename):
  if not os.path.isfile(filename):
    raise FileNotFoundError(f"The file '{filename}' was not found.")
  
  timestamps = []
  voltages = []
  classifications = []
  
  try:
    with open(filename, 'r') as f:
      # Ler primeira linha (cabeçalho)
      header = f.readline().strip()
      columns = [col.strip() for col in header.split(',')]
      
      # Verificar se as colunas necessárias existem
      if 'timestamp_us' not in columns:
        raise ValueError("Column 'timestamp_us' not found in CSV file")
      if 'voltage' not in columns:
        raise ValueError("Column 'voltage' not found in CSV file")
      
      timestamp_idx = columns.index('timestamp_us')
      voltage_idx = columns.index('voltage')
      classification_idx = columns.index('classification') if 'classification' in columns else None
      
      # Ler dados
      for line_num, line in enumerate(f, start=2):
        line = line.strip()
        if not line:  # Pular linhas vazias
          continue
            
        parts = line.split(',')
        if len(parts) != len(columns):
          raise ValueError(f"Line {line_num}: Expected {len(columns)} columns, got {len(parts)}")
        
        try:
          timestamp = int(float(parts[timestamp_idx]))
          voltage = float(parts[voltage_idx])
          classification = parts[classification_idx].strip() if classification_idx is not None else 'N'
          
          timestamps.append(timestamp)
          voltages.append(voltage)
          classifications.append(classification)
        except ValueError as e:
          raise ValueError(f"Line {line_num}: Invalid data format - {e}")
    
    if not timestamps:
        raise ValueError("CSV file contains no data")
    
    return (np.array(voltages, dtype=np.float32), 
            np.array(timestamps, dtype=np.int64),
            classifications if classification_idx is not None else None)
      
  except IOError as e:
    raise ValueError(f"Error reading CSV file: {e}")


def detect_file_type(filename):
  _, ext = os.path.splitext(filename.lower())
  if ext == '.csv':
    return 'csv'
  else:
    return 'binary'


def plot_ecg(voltages, timestamps, classifications, filename, file_type):
  if file_type == 'csv':
    time_sec = (timestamps - timestamps[0]) / 1e6
  else:
    time_sec = (timestamps - timestamps[0]) / 1e6

  fig, ax = plt.subplots(figsize=(15, 6), dpi=100)

  # Plot do sinal ECG
  ecg_line = ax.plot(time_sec, voltages, label="ECG Signal", linewidth=1, color='blue')

  # Preparar handles e labels para controle da ordem da legenda
  legend_handles = [ecg_line[0]]
  legend_labels = ["ECG Signal"]

  # Se houver classificações, marcar os pontos de interesse
  if classifications is not None:
    # Definir cores e marcadores para cada tipo de classificação
    classification_styles = {
      'P': {'color': 'green', 'marker': 'o', 'size': 35, 'label': 'P Wave'},
      'Q': {'color': 'darkgoldenrod', 'marker': 'o', 'size': 40, 'label': 'Q Wave'},
      'R': {'color': 'red', 'marker': 'o', 'size': 50, 'label': 'R Wave'},
      'S': {'color': 'darkblue', 'marker': 'o', 'size': 40, 'label': 'S Wave'},
      'T': {'color': 'purple', 'marker': 'o', 'size': 35, 'label': 'T Wave'}
    }
    
    # Agrupar pontos por classificação para eficiência
    classification_groups = {}
    for i, classification in enumerate(classifications):
      if classification in classification_styles:  # P, Q, R, S, T
        if classification not in classification_groups:
          classification_groups[classification] = {'times': [], 'voltages': []}
        classification_groups[classification]['times'].append(time_sec[i])
        classification_groups[classification]['voltages'].append(voltages[i])
    
    # Plotar na ordem desejada: P, Q, R, S, T
    desired_order = ['P', 'Q', 'R', 'S', 'T']
    for classification in desired_order:
      if classification in classification_groups:
        data = classification_groups[classification]
        style = classification_styles[classification]
        scatter = ax.scatter(data['times'], data['voltages'], 
                          c=style['color'], 
                          marker=style['marker'], 
                          s=style['size'],
                          alpha=0.9,
                          zorder=3)
        legend_handles.append(scatter)
        legend_labels.append(style['label'])
    
    if len(classification_groups) > 0:
      print(f"Pontos marcados por classificação:")
      for classification in desired_order:
        if classification in classification_groups:
          data = classification_groups[classification]
          print(f"  {classification}: {len(data['times'])} pontos")
    else:
      print("Nenhum ponto P, Q, R, S ou T encontrado para marcação")

  ax.axhline(0, color="black", linestyle='-', alpha=0.3)
  ax.set_xlabel("Time (s)")
  ax.set_ylabel("Voltage (V)")
  ax.set_title(f'ECG Data from {os.path.basename(filename)}')
  
  # Controlar ordem da legenda explicitamente
  ax.legend(legend_handles, legend_labels)
  ax.grid(True, alpha=0.3)

  plt.tight_layout()
  plt.show()


def main():
  parser = argparse.ArgumentParser(description="Plot ECG data from binary or CSV file with classification markers")
  parser.add_argument("filename", help="Caminho para o arquivo de entrada (binário ou CSV)")
  parser.add_argument("--type", choices=['binary', 'csv', 'auto'], default='auto',
                      help="Tipo de arquivo (auto detecta baseado na extensão)")
  args = parser.parse_args()

  try:
    if args.type == 'auto':
      file_type = detect_file_type(args.filename)
    else:
      file_type = args.type

    print(f"Reading {file_type} file: {args.filename}")

    if file_type == 'csv':
      voltages, timestamps, classifications = read_csv_file(args.filename)
    else:
      voltages, timestamps, classifications = read_binary_file(args.filename)

    print(f"Successfully loaded {len(voltages)} samples")
    print(f"Voltage range: {np.min(voltages):.3f}V to {np.max(voltages):.3f}V")
    print(f"Duration: {(timestamps[-1] - timestamps[0]) / 1e6:.2f} seconds")
    
    if classifications is not None:
      marked_points = len([c for c in classifications if c in ['R', 'Q', 'S', 'P', 'T']])
      print(f"Classifications available: {marked_points} marked points (R, Q, S, P, T)")
    else:
      print("No classification data available (binary file or CSV without classification column)")

    plot_ecg(voltages, timestamps, classifications, args.filename, file_type)

  except FileNotFoundError as e:
    print(f"Error: {e}")
  except ValueError as e:
    print(f"Error: {e}")
  except Exception as e:
    print(f"Unexpected error: {e}")


if __name__ == "__main__":
  main()