# Windows Serial Recovery

This board uses a Silicon Labs CP210x USB-UART bridge. Windows may show the
device as present in Device Manager while Python, ESP-IDF, and esptool cannot
open the COM port.

## Symptoms

- `Get-CimInstance Win32_SerialPort` lists the CP210x device.
- `[System.IO.Ports.SerialPort]::GetPortNames()` lists a COM port.
- Opening the port fails with `FileNotFoundError`.
- ESP-IDF flashing fails with:

```text
Could not open COM20, the port is busy or doesn't exist.
FileNotFoundError(2, 'The system cannot find the file specified.')
```

This happened with both `COM150` and `COM20`.

## Check Current Port

Run these in PowerShell:

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Name,PNPDeviceID
```

Test whether Python can actually open the port:

```powershell
@'
import serial
for port in ["COM20", "COM3", r"\\.\COM20", r"\\.\COM3"]:
    try:
        s = serial.Serial(port, 115200, timeout=1)
        print(port, "OPEN OK")
        s.close()
    except Exception as e:
        print(port, "FAIL", repr(e))
'@ | python -
```

If the port is listed but cannot be opened, continue below.

## Recovery That Worked

Removing the CP210x device instance and scanning devices again recovered the
serial port. It came back as `COM3` and could be opened normally.

```powershell
$id = (Get-PnpDevice -Class Ports | Where-Object { $_.FriendlyName -like '*CP210x*' }).InstanceId
& 'C:\Windows\System32\pnputil.exe' /remove-device $id
Start-Sleep -Seconds 3
& 'C:\Windows\System32\pnputil.exe' /scan-devices
Start-Sleep -Seconds 4
[System.IO.Ports.SerialPort]::GetPortNames()
```

Then verify the new port:

```powershell
@'
import serial
s = serial.Serial("COM3", 115200, timeout=1)
print("OPEN OK")
s.close()
'@ | python -
```

Flash with the recovered port:

```powershell
. E:\Espressif\v6.0.1\esp-idf\export.ps1
idf.py -p COM3 flash
```

## Notes

- Physical unplug/replug and disable/enable did not always fix this state.
- `Get-PnpDevice` reported `CM_PROB_NONE` and `Status: OK` even while the COM
  port could not be opened.
- The successful recovery changed the port from `COM20` to `COM3`.
- Prefer the current port shown by `GetPortNames()` instead of assuming a fixed
  COM number.
