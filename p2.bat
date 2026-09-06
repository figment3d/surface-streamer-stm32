@echo off
call p.bat
python -c "import socket; u=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); u.settimeout(10); u.bind(('0.0.0.0',10000)); print('UDP waiting...'); data,addr=u.recvfrom(1024); print('UDP_READY',addr[0],data.decode(errors='replace')); u.close()"