---
applyTo: "website/**/*.*"
---

# GitHub Copilot Instructions for website

## Project Overview

This is the frontend web application for the marble track system. It is built with SolidJs (Not React!), TypeScript, and Vite. The app communicates with the ESP32 firmware via WebSocket and provides a UI for device control and monitoring.

## Coding Guidelines

- Use TypeScript for all source files
- Organize code by feature: components, hooks, interfaces, stores, utils, assets.
- Use CSS modules for component styling with BEM naming conventions
- All device types should have a corresponding component in `components/devices/`.
- Update interfaces in `interfaces/` when adding new device types or message formats.
- Use the `stores/` directory for global state management (e.g., logger, websocketStore).
- Never destructure component props, use a argument props and in the TSX use {props.field} (See SolidJS)
- Never destructure hook returned store props, as it removes the reactivity of store

## File Structure

- `src/`: Main source code
  - `assets/`: file that must be publicly accessible from the web (favicon, images)
  - `components/`: UI components (Header, WebSocketMessages, WebSocketSender, devices)
  - `hooks/`: Custom SolidJS hooks
  - `interfaces/`: TypeScript interfaces
  - `stores/`: State management
  - `utils/`: Utility functions
  - `assets/`: Images and icons
- `index.html`: App entry point
- `vite.config.ts`: Vite configuration
- `tsconfig.json`: TypeScript configuration
- `eslint.config.js`: ESLint configuration

## Scripts

- Install dependencies: `npm install`
- Start dev server: `npm run dev`
- Build for production: `npm run build`
- Preview production build: `npm run preview`
- Validating formatting and linting rules: `npm run check`
- Fixing formatting and lint rules: `npm run fix`
- Deploy website on esp32: `../deploy-website.ps1`

## Best Practices

- Keep readme and instructions up to date.
- Always run `npm run fix` after a website change

## Testing

- The website started with `npm run dev` is running on `localhost:3000`.
- The website when uploaded to the ESP32 with `../deploy-website.ps1` is available on the ESP32 device on `marble-track.local`.
- To debug the website, you can test the website and debug it with the MCP Chrome Devtools.
