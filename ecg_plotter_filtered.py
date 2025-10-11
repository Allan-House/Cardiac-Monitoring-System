import os
import struct
import numpy as np
import matplotlib.pyplot as plt
import argparse
from scipy import signal

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

def calculate_sampling_rate(timestamps):
    """Calcula a taxa de amostragem baseada nos timestamps"""
    time_diffs = np.diff(timestamps)
    avg_time_diff = np.mean(time_diffs) / 1e6  # converter para segundos
    fs = 1.0 / avg_time_diff
    return fs

def notch_filter(signal_data, fs, freq=60.0, quality=30):
    """
    Filtro notch para remover interferência da rede elétrica
    
    Args:
        signal_data: sinal de entrada
        fs: frequência de amostragem
        freq: frequência a ser removida (50Hz para Europa, 60Hz para Brasil/EUA)
        quality: fator de qualidade (Q) do filtro
    """
    nyquist = fs / 2
    freq_normalized = freq / nyquist
    
    # Criar filtro notch (rejeita banda)
    b, a = signal.iirnotch(freq_normalized, quality)
    filtered_signal = signal.filtfilt(b, a, signal_data)
    
    return filtered_signal

def apply_notch_filter(voltages, timestamps, powerline_freq=60, apply_harmonic=True):
    """
    Aplica filtro notch para remover interferência da rede elétrica
    
    Args:
        voltages: sinal de voltagem
        timestamps: timestamps
        powerline_freq: frequência da rede elétrica (50Hz ou 60Hz)
        apply_harmonic: se True, aplica filtro também na frequência harmônica
    """
    fs = calculate_sampling_rate(timestamps)
    print(f"Taxa de amostragem detectada: {fs:.2f} Hz")
    
    # Filtro notch na frequência fundamental
    signal_filtered = notch_filter(voltages, fs, freq=powerline_freq, quality=30)
    
    # Filtro notch adicional para harmônicas (120Hz para 60Hz, 100Hz para 50Hz)
    if apply_harmonic:
        harmonic_freq = 2 * powerline_freq
        signal_filtered = notch_filter(signal_filtered, fs, freq=harmonic_freq, quality=30)
        print(f"Filtros aplicados: {powerline_freq}Hz e {harmonic_freq}Hz")
    else:
        print(f"Filtro aplicado: {powerline_freq}Hz")
    
    return signal_filtered, fs

def plot_ecg_comparison(voltages_raw, voltages_filtered, timestamps, filename, fs):
    """Plota comparação entre sinal original e filtrado"""
    time_sec = (timestamps - timestamps[0]) / 1e6
    
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(15, 12), dpi=100)
    
    # Sinal original
    ax1.plot(time_sec, voltages_raw, label="ECG Original", linewidth=0.8, color='blue', alpha=0.7)
    ax1.axhline(0, color="black", linestyle='-', alpha=0.3)
    ax1.set_ylabel("Voltage (V)")
    ax1.set_title(f'ECG Original - {os.path.basename(filename)} (fs = {fs:.1f} Hz)')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Sinal filtrado
    ax2.plot(time_sec, voltages_filtered, label="ECG Filtrado (Notch)", linewidth=0.8, color='red')
    ax2.axhline(0, color="black", linestyle='-', alpha=0.3)
    ax2.set_ylabel("Voltage (V)")
    ax2.set_title('ECG após Filtro Notch')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Comparação sobreposta
    ax3.plot(time_sec, voltages_raw, label="Original", linewidth=0.8, color='blue', alpha=0.6)
    ax3.plot(time_sec, voltages_filtered, label="Filtrado", linewidth=0.8, color='red')
    ax3.axhline(0, color="black", linestyle='-', alpha=0.3)
    ax3.set_xlabel("Time (s)")
    ax3.set_ylabel("Voltage (V)")
    ax3.set_title('Comparação: Original vs Filtrado')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.show()

def plot_frequency_spectrum(voltages_raw, voltages_filtered, fs, powerline_freq):
    """Plota espectro de frequência antes e depois da filtragem"""
    freqs_raw = np.fft.fftfreq(len(voltages_raw), 1/fs)
    fft_raw = np.fft.fft(voltages_raw)
    
    freqs_filtered = np.fft.fftfreq(len(voltages_filtered), 1/fs)
    fft_filtered = np.fft.fft(voltages_filtered)
    
    # Plotar apenas frequências positivas
    positive_freqs_raw = freqs_raw[:len(freqs_raw)//2]
    positive_fft_raw = np.abs(fft_raw[:len(fft_raw)//2])
    
    positive_freqs_filtered = freqs_filtered[:len(freqs_filtered)//2]
    positive_fft_filtered = np.abs(fft_filtered[:len(fft_filtered)//2])
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(15, 8), dpi=100)
    
    # Espectro original
    ax1.semilogy(positive_freqs_raw, positive_fft_raw, color='blue', alpha=0.7)
    ax1.set_ylabel("Amplitude")
    ax1.set_title("Espectro de Frequência - Sinal Original")
    ax1.grid(True, alpha=0.3)
    ax1.axvline(x=powerline_freq, color='red', linestyle='--', alpha=0.7, label=f'{powerline_freq}Hz')
    ax1.axvline(x=2*powerline_freq, color='orange', linestyle='--', alpha=0.7, label=f'{2*powerline_freq}Hz')
    ax1.legend()
    ax1.set_xlim(0, 200)
    
    # Espectro filtrado
    ax2.semilogy(positive_freqs_filtered, positive_fft_filtered, color='red')
    ax2.set_xlabel("Frequência (Hz)")
    ax2.set_ylabel("Amplitude")
    ax2.set_title("Espectro de Frequência - Sinal Filtrado (Notch)")
    ax2.grid(True, alpha=0.3)
    ax2.axvline(x=powerline_freq, color='red', linestyle='--', alpha=0.7, label=f'{powerline_freq}Hz')
    ax2.axvline(x=2*powerline_freq, color='orange', linestyle='--', alpha=0.7, label=f'{2*powerline_freq}Hz')
    ax2.legend()
    ax2.set_xlim(0, 200)
    
    plt.tight_layout()
    plt.show()

def main():
    parser = argparse.ArgumentParser(description="Plot ECG with notch filter from binary file")
    parser.add_argument("filename", help="Caminho para o arquivo binário de entrada")
    parser.add_argument("--powerline", "-p", type=int, choices=[50, 60], default=60,
                        help="Frequência da rede elétrica (50Hz ou 60Hz). Padrão: 60Hz")
    parser.add_argument("--no-harmonic", action="store_true",
                        help="Não aplicar filtro na frequência harmônica")
    parser.add_argument("--show-spectrum", "-s", action="store_true",
                        help="Mostrar espectro de frequência")
    parser.add_argument("--filter-only", "-f", action="store_true",
                        help="Mostrar apenas o sinal filtrado")
    
    args = parser.parse_args()
    
    try:
        print("Lendo arquivo...")
        voltages_raw, timestamps = read_binary_file(args.filename)
        
        print("Aplicando filtro notch...")
        voltages_filtered, fs = apply_notch_filter(
            voltages_raw, 
            timestamps, 
            args.powerline,
            apply_harmonic=not args.no_harmonic
        )
        
        if args.filter_only:
            # Plotar apenas sinal filtrado
            time_sec = (timestamps - timestamps[0]) / 1e6
            fig, ax = plt.subplots(figsize=(15, 6), dpi=100)
            ax.plot(time_sec, voltages_filtered, label="ECG Filtrado (Notch)", linewidth=1, color='red')
            ax.axhline(0, color="black", linestyle='-', alpha=0.3)
            ax.set_xlabel("Time (s)")
            ax.set_ylabel("Voltage (V)")
            ax.set_title(f'ECG Filtrado - {os.path.basename(args.filename)} (fs = {fs:.1f} Hz)')
            ax.legend()
            ax.grid(True, alpha=0.3)
            plt.show()
        else:
            # Plotar comparação
            plot_ecg_comparison(voltages_raw, voltages_filtered, timestamps, args.filename, fs)
        
        if args.show_spectrum:
            print("Gerando espectro de frequência...")
            plot_frequency_spectrum(voltages_raw, voltages_filtered, fs, args.powerline)
            
    except Exception as e:
        print(f"Erro: {e}")

if __name__ == "__main__":
    main()