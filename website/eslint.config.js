import js from '@eslint/js';
import tseslint from 'typescript-eslint';
import solid from 'eslint-plugin-solid/configs/typescript';
import prettierConfig from 'eslint-config-prettier';

export default tseslint.config(
  js.configs.recommended,
  ...tseslint.configs.recommended,
  {
    ...solid,
    files: ['**/*.{ts,tsx}'],
    languageOptions: {
      parser: tseslint.parser,
      parserOptions: {
        project: './tsconfig.json',
      },
    },
    rules: {
      // SolidJS specific rules - more lenient
      'solid/reactivity': 'off', // Often too strict for simple cases
      'solid/no-destructure': 'warn',
      'solid/prefer-for': 'warn',
      
      // TypeScript rules
      '@typescript-eslint/no-unused-vars': ['warn', { 
        argsIgnorePattern: '^_',
        varsIgnorePattern: '^_',
        ignoreRestSiblings: true
      }],
      '@typescript-eslint/explicit-function-return-type': 'off',
      '@typescript-eslint/explicit-module-boundary-types': 'off',
      '@typescript-eslint/no-explicit-any': 'off', // Allow any for now
      
      // General JavaScript/TypeScript rules
      'no-console': 'off', // Allow console for debugging
      'prefer-const': 'error',
      'no-var': 'error',
      'eqeqeq': 'error',
      'curly': 'error',
      
      // Import/Export rules
      'no-unused-vars': 'off', // Disabled in favor of @typescript-eslint/no-unused-vars
    },
  },
  {
    files: ['**/*.js'],
    ...js.configs.recommended,
  },
  prettierConfig, // Must be last to override other configs
  {
    ignores: [
      'dist/**',
      'node_modules/**',
      '*.config.js',
      'vite.config.ts',
      '../esp32_ws/data/**',
    ],
  }
);
