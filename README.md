# Programmation Réseaux - Awalé Game
An implementation of a client/server application of the Awalé Game \
by Ewan GAROUX, Nathan AKNIN et Stefan SEVERIN.

# Compilation
Pull the repository and type `make`

# Start the application
Switch into the folder `/bin` and use the following commands 

**Server side :**
`./server`

**Client side :**
`./client [adress of the server] [username]`

# List of commands

| Command | Action |
| --- | --- |
| /accept | Accept an invitation to a challenge |
| /bio | Modify your bio |
| /challenge [username] | Challenge a player to a game |
| /end | End/forfeit an ongoing game |
| /friend <add &#124; remove> [username] | Add or remove a friend allowed to spectate when private |
| /friends | List your friends |
| /help | Help menu with all of the supported commands |
| /games | List every ongoing games |
| /observe [challenge] | Observe ongoing games |
| /players | List every usernames of players connected |
| /private <on &#124; off> | Enable/disable private mode for your matches |
| /quit | Quit observer mode|
| /ranking <me &#124; [number]> | See the player ranking |
| /refuse | Decline an invitation to a challenge |
| /t [username] [message]| Start a chat with a user |
| /viewgame [number] | Shows the saved game |
| /whois [username] | Consult a player's bio |


