# ESLint Configuration

This project uses ESLint with flat config style for modern JavaScript/TypeScript linting.

## Features

- **Flat Config Style**: Uses the new `eslint.config.js` format
- **TypeScript Support**: Full TypeScript integration with `typescript-eslint`
- **SolidJS Support**: Specific rules for SolidJS components
- **VSCode Integration**: Automatic formatting and error highlighting

## Available Scripts

```bash
# Run ESLint
npm run lint

# Fix auto-fixable issues
npm run lint:fix

# Type check only
npm run type-check

# Run both type check and lint
npm run check
```

## Configuration

The configuration is defined in `eslint.config.js` with the following features:

- **Rules**: Balanced between strictness and practicality
- **SolidJS Rules**: `solid/no-destructure`, `solid/prefer-for`
- **TypeScript Rules**: Unused variables warnings, no explicit any warnings disabled
- **General Rules**: Prefer const, no var, strict equality

## VSCode Integration

The `.vscode/settings.json` file enables:
- Automatic ESLint fixing on save
- Real-time error highlighting
- Format on save integration

## Ignored Files

The following are automatically ignored:
- `dist/**` - Build output
- `node_modules/**` - Dependencies  
- `*.config.js` - Configuration files
- `vite.config.ts` - Vite configuration
- `../esp32_ws/data/**` - ESP32 build output
