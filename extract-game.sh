#!/bin/sh
# Usage: extract-game.sh <CHD file> <PIC file> <Destination folder> <DIMM size>

# Create and clear temporary folder

rm -rf /tmp/chihiro/
mkdir /tmp/chihiro/

# Find decryption key and loader

key=`./parse-securityic "$2" key`
loader=`./parse-securityic "$2" loader`

# Extract CHD

/usr/share/sdlmame/chdman extractcd -i "$1" -o /tmp/chihiro/gdrom.toc

# Extract track-3 which contains game data

toc2cue /tmp/chihiro/gdrom.toc /tmp/chihiro/gdrom.cue
(cd /tmp/chihiro/; bin2iso gdrom.cue -t 03) # bin2iso sucks.. can't specify input directory
rm -f /tmp/chihiro/gdrom.bin /tmp/chihiro/gdrom.cue

# Extract GDROM

./extract-gdrom /tmp/chihiro/gdrom-03.iso /tmp/chihiro/gdrom/
rm -f /tmp/chihiro/gdrom-03.iso

# Find the FATX image by reading the loader

fatx=`./parse-loader /tmp/chihiro/gdrom/$loader fatx`

# Decrypt the FATX image

./decrypt /tmp/chihiro/gdrom/$fatx /tmp/chihiro/image.fatx $key
rm -rf /tmp/chihiro/gdrom/

# Now extract it

./extract-fatx /tmp/chihiro/image.fatx "$3" "$4"

# Delete temporary files

rm -rf /tmp/chihiro/
