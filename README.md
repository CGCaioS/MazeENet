# Maze ENet Example

> Client-server implementation for reporting other player's position at map.

Clients expects server to be running when they are launched.

ENet helper classes taken and reduced from (https://github.com/kbirk/enet-example)

## Usage

Build the VS solution which contains the Level Editor, the Maze Game and the ENet Server.

Launch the server and at least two Maze Clients.

When the player is moved on a client, its position is broadcast to other clients and they show up in the map as a hash sign (#)

