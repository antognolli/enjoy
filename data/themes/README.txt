# These are the parts and events that Enjoy expects in the theme:

# Parts that must be in the theme:
# --------------------------------
PART = gui.text.title
PART = gui.text.artist
PART = gui.text.album
PART = gui.text.filename
PART = gui.text.time_total
PART = gui.text.time_elapsed
PART = gui.text.time_remain

# Signals received from the theme:
# --------------------------------
SIGNAL_RCV = gui,action,play
SIGNAL_RCV = gui,action,pause
SIGNAL_RCV = gui,action,stop
SIGNAL_RCV = gui,action,rewind
SIGNAL_RCV = gui,action,forward
SIGNAL_RCV = gui,action,previous
SIGNAL_RCV = gui,action,next

# Signals sent to the theme:
# --------------------------
SIGNAL_SENT = gui,action,playing
SIGNAL_SENT = gui,action_stopped

# Messages received from the theme:
# ---------------------------------
MSG_RCV_ID1 = EDJE_MESSAGE_FLOAT # Progress changed to <float>
MSG_RCV_ID2 = EDJE_MESSAGE_FLOAT # Volume changed to <float>

# Messages sent to the theme:
# ---------------------------
MSG_SENT_ID1 = EDJE_MESSAGE_FLOAT # Progress update to <float>
MSG_SENT_ID2 = EDJE_MESSAGE_FLOAT # Volume update to <float>
