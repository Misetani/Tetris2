# Tetris2
C++ Tetris rewritten using FSM

## FSM
```
          SHIFTING
             ^
             |
INITIAL -> MOVING -> PAUSE          GAME_OVER (may happen by attaching a figure or by pressing a q button)
             |
             v
          ATTACHING
```

MOVING state is the main state of the game in which all user input is handled. That is user
can restart, pause, unpause, quit or move figure left, right or down. Every dozen or so iterations
automatic shifting happens and if falling figure reaches bottom of the playing field or other figures
attaching happens. If coordinates of the attached figure happens to be at the very top of the playing field
(y <= 0) game ends.
