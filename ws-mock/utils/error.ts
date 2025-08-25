// function to send WS error
export function sendError(ws: import("ws").WebSocket, message: string) {
  console.error(message);
  ws.send(JSON.stringify({ type: "error", msg: message }));
}
