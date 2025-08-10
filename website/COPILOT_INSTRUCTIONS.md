# GitHub Copilot Instructions for website

## Project Overview
This is the frontend web application for the marble track system. It is built with SolidJs (Not React!), TypeScript, and Vite. The app communicates with the ESP32 firmware via WebSocket and provides a UI for device control and monitoring.

## Coding Guidelines
- Use TypeScript for all source files
- Organize code by feature: components, hooks, interfaces, stores, utils, assets.
- Use CSS modules for component styling with BEM naming convetions
- All device types should have a corresponding component in `components/devices/`.
- Update interfaces in `interfaces/` when adding new device types or message formats.
- Use the `stores/` directory for global state management (e.g., logger, websocketStore).

## File Structure
- `src/`: Main source code
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

## Development & Build
- Install dependencies: `npm install`
- Start dev server: `npm run dev`
- Build for production: `npm run build`
- Preview production build: `npm run preview`
- Validating formatting and linting rules: `npm run check`
- Fixing formatting and lint rules: `npm run fix`

## Troubleshooting
- If build or lint errors occur, check TypeScript types and ESLint output.
- Use the browser console .

## Best Practices
- Use descriptive names for components, props, and state variables.
- Document new device types and UI patterns in this file.
- Keep UI responsive and accessible.

---
For further assistance, use GitHub Copilot in VS Code and refer to this file for project-specific instructions.
