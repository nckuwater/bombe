# Bombe
An Enigma cracker based on the mechanism of the bombe machine.

## Settings
set the known plain-cipher pair in bombe_pc.txt.
rotors_config.txt contains the letter config for each rotor.
reflector_config.txt contains the reflector letter config.

## Crack Process
It will try to form the bombe menu from given plain-cipher pairs,
then check one by one by running all 26 letters in all possible rotor settings, 
run every letter in Enigma with the steps of the loop,
if any letter match it's output letter, then it's a possible rotor setting,
and we can get the possible plugboard setting as \[the letter, start letter of the loop],
if any loop fails or a plugboard conflict occurs, the rotor setting is wrong, 
try the next one.

Note:
This implementation doesn't support ring settings
