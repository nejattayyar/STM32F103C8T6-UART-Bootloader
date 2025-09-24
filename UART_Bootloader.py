import serial
import time
import math

# Seri port ve baud rate
# Timeout değerini 1 saniyeye düşürerek daha hızlı hata tespiti yapabiliriz
ser = serial.Serial('COM6', 115200, timeout=1)


def wait_for_ready():
    """Bootloader'dan 'READY' mesajını bekler."""
    print("Bootloader'dan 'READY' onayı bekleniyor...")
    start_time = time.time()
    while True:
        line = ser.readline()
        if b'READY' in line:
            print("Bootloader hazır.")
            return True
        if time.time() - start_time > 10:  # 10 saniye bekle
            print("Bootloader onayı alınamadı. Zaman aşımı.")
            return False


def send_firmware(file_path):
    """Firmware dosyasını UART üzerinden gönderir."""
    with open(file_path, 'rb') as f:
        fw_data = f.read()

    fw_size = len(fw_data)
    print(f"Firmware boyutu: {fw_size} bayt")

    # 4 byte hizalama için padding ekle
    padding = (4 - (fw_size % 4)) % 4
    if padding:
        fw_data += b'\xFF' * padding
        print(f"Padding eklendi: {padding} bayt")

    # Header oluştur ve gönder: 4 byte boyut, 4 byte CRC (şimdilik 0)
    header = fw_size.to_bytes(4, 'little') + b'\x00\x00\x00\x00'
    ser.write(header)
    print("Header gönderildi.")

    # Bootloader'dan header onayını bekle
    response = ser.read_until(b'OK\n')
    if b'OK' not in response:
        print(f"Header onayı alınamadı! Gelen yanıt: {response.decode('utf-8')}")
        return False
    print("Header onaylandı.")
    time.sleep(0.05)

    # Her bir chunk'ı gönder ve onayını bekle
    # Veri gönderme döngüsü
    CHUNK_SIZE = 256
    for i in range(0, len(fw_data), CHUNK_SIZE):

        # YENİ ADIM: Bootloader'dan 'SEND_CHUNK' komutu bekle
        print(f"Bootloader'dan Chunk {i // CHUNK_SIZE} gönderme izni bekleniyor...")
        permission = ser.read_until(b'SEND_CHUNK\n')

        if b'SEND_CHUNK' not in permission:
            print(f"Gönderme izni alınamadı. Yanıt: {permission.decode('utf-8')}")
            # Hata varsa, bootloader'ın gönderdiği hata mesajını yakalamak için burada bir kontrol daha yapılabilir.
            return False

        print(f"İzin alındı. Chunk {i // CHUNK_SIZE} gönderiliyor.")

        chunk = fw_data[i:i + CHUNK_SIZE]
        ser.write(chunk)

        # Şimdi chunk onayını bekle
        response = ser.read_until(b'OK\n')
        if b'OK' not in response:
            print(f"Chunk {i // CHUNK_SIZE} onayı alınamadı. Gelen yanıt: {response.decode('utf-8')}")
            return False

    # Son olarak, başarılı yükleme mesajını bekle
    response = ser.read_until(b'SUCCESS\n')
    if b'SUCCESS' not in response:
        print(f"Yükleme tamamlanmadı! Son yanıt: {response.decode('utf-8')}")
        return False

    print("Firmware başarıyla yüklendi!")
    return True


# --- Ana akış ---
if wait_for_ready():
    if send_firmware('jump_to_app2.bin'):
        print("İşlem Başarılı.")
    else:
        print("İşlem Başarısız.")

ser.close()