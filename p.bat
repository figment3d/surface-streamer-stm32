@echo off
python -c "import serial; s=serial.Serial('COM7',115200,timeout=2); s.write(b'I2C_STATUS\r'); print(s.readline().decode(errors='replace').strip()); s.write(b'SPI_STATUS\r'); print(s.readline().decode(errors='replace').strip()); s.close()"
pause