## Overview

This folder contains support packages for various ESP32 based development boards.

Build any of the examples from this sdk by providing AUDIO_BOARD_PATH for any of the board.

For example command below will build voice assistant example for `LyraT` board:

```
make -j8 AUDIO_BOARD_PATH=<path_to_this folder/lyrat/audio_board/audio_board_lyrat/>
```

You may as well create package for your own board in this fashion and provide path as example above to build for that board.
