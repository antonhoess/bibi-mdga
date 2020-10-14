# Bibi Man, Don't Get Angry
A simple Man, Don't Get Angry version for my two beloved little ones (Bibis).

It's implemented in C using Gtk+ and libcanberra for playing sounds.

A primitve computer enemy and record/play (store/load) functionality is implemented.

## Install
```bash
sudo apt-get install libcairo2-dev
sudo apt-get install libpthread-stubs0-dev
sudo apt-get install libcanberra-dev
```

## Compile and run
```bash
make
./bibi-mdga
```

## Here's a screenshot on how it looks so far
![Bibi MDGA in action](images/bibi-mdga.png "Bibi MDGA in action.")

## How to play
### Choose player
Use the keys 1..4 for the players 1..4 respectively. When pressing one of these buttons the corresponding player toggles between these states:
* Not active
* Human
* Computer (primitive AI)

### Make a turn
The game rotates between the players. For each player, there are two steps:
* Just press enter to roll (see the die in the center).
* Then, if there's even a joice, click on one of the now dark colored circles to make the move. If just instead press enter again, the program decides, which figure to move.

### Play the computer
When it's the computer's turn, just press enter twice for roll and move.
