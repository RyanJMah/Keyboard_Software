# pylinter: skip-file

import click
import serial
import keyboard

@click.command()
@click.option("--com-port", "com_port", default = "COM3", show_default = True, help = "COM[n] - COM port of device")
@click.option("--baud-rate", "baud_rate", default = 9600, show_default = True, help = "Baud rate of device")
@click.option("--verbose", "verbose", flag_value = "verbose", help = "print keystrokes to console")
def driver(com_port, baud_rate, verbose):
    port = com_port
    rate = baud_rate

    serial_port = serial.Serial(port, rate)
    if verbose == "verbose":
        while True:
            try:
                keystroke = serial_port.read().decode()
                click.echo(keystroke)
                keyboard.press_and_release(keystroke)
            
            except UnicodeDecodeError:
                pass
            except KeyboardInterrupt:
                pass
    else:
        while True:
            try:
                keystroke = serial_port.read().decode()
                keyboard.press_and_release(keystroke)

            except UnicodeDecodeError:
                pass
            except KeyboardInterrupt:
                pass


