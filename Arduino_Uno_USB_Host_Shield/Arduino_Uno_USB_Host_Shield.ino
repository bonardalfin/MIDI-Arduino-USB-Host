#include <usbh_midi.h>
#include <usbhub.h>

// Inisialisasi USB Host dan USB MIDI
USB Usb;
USBH_MIDI MIDI(&Usb);

// Variabel untuk menyimpan nilai CC (1 sampai 10)
uint8_t ccValue = 1;
unsigned long lastSendTime = 0; // Menggunakan millis() agar USB.Task() tidak terganggu delay

void setup() {
  Serial.begin(115200);
  
  if (Usb.Init() == -1) {
    Serial.println("Gagal menginisialisasi USB Host Shield!");
    while (1); // Hentikan program jika shield tidak terdeteksi
  }
  
  Serial.println("USB Host Shield berhasil diinisialisasi.");
  Serial.println("Menunggu perangkat MIDI USB dicolokkan...");
}

void loop() {
  // Selalu jalankan tugas USB Host (wajib dijalankan terus-menerus tanpa terinterupsi)
  Usb.Task();

  // CARA YANG BENAR: Mengecek apakah class MIDI aktif dan terhubung
  if (MIDI) {
    
    // Menggunakan millis() alih-alih delay(1000) agar komunikasi USB tetap lancar
    if (millis() - lastSendTime >= 1000) {
      lastSendTime = millis();

      // Konfigurasi Parameter MIDI
      uint8_t midiChannel = 0; // Channel 1 (0 = Ch 1, 1 = Ch 2, dst)
      uint8_t ccNumber = 1;    // CC# 1
      
      // Mengirim data MIDI CC
      sendMIDI_CC(midiChannel, ccNumber, ccValue);

      // Print status ke Serial Monitor
      Serial.print("Mengirim MIDI CC#1 | Channel 1 | Value: ");
      Serial.println(ccValue);

      // Naikkan nilai, jika lewat dari 10 balik lagi ke 1
      ccValue++;
      if (ccValue > 10) {
        ccValue = 1;
      }
    }
  }
}

// Fungsi pembantu untuk membungkus pengiriman pesan CC
void sendMIDI_CC(uint8_t chan, uint8_t control, uint8_t val) {
  uint8_t buf[3];
  buf[0] = 0xB0 | chan;   // 0xB0 adalah status byte untuk Control Change
  buf[1] = control;       // Nomor CC (CC#1)
  buf[2] = val;           // Nilai CC (1-10)
  
  // Kirim data mentah melalui USB MIDI
  MIDI.SendData(buf);
}