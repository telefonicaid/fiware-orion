#!/usr/bin/env python

import sys
import websocket

msg = 0;

def on_message(ws, message):
    print message

    global msg
    msg -= 1
    if (msg == 0):
        ws.close()

def on_error(ws, error):
    print error

def on_close(ws):
    print "Connection closed"

def on_open(ws):
    ws.send(sys.argv[3])

def show_help(reason):
    print "\n" + reason
    print "Usage:"
    print "\t" + sys.argv[0] + " URI protocol payload [responses]\n"
    print "* URI      : websocket server URI."
    print "* protocol : protocol to use in this session."
    print "* payload  : payload to send."
    print "* responses: number of responses to wait before quit. By default is 1"
    print ""


def check_params():
    err = len(sys.argv) < 4
    if (err):
        show_help("Error, missing arguments")
    else:
        global msg
        if (len(sys.argv) == 5):
            try:
                msg = int(sys.argv[4])
            except Exception:
                show_help("Wrong arguments, responses must be a number.")
                return False
        else:
            msg = 1

    return not(err)

if __name__ == "__main__":
    if (check_params()):
        ws = websocket.WebSocketApp(sys.argv[1],
                                    subprotocols=[sys.argv[2]],
                                    on_message = on_message,
                                    on_error = on_error,
                                    on_close = on_close,
                                    on_open = on_open)
        ws.run_forever()
