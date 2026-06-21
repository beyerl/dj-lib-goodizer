import { Page, expect } from '@playwright/test';

// Shape of window.__djState published by the WASM bridge (app/WasmBridge.cpp)
// and by the local mock.
export interface DjState {
  ready: boolean;
  version: string;
  windowTitle: string;
  rowCount: number;
  currentTab: number;
  tabName: string;
  statusText: string;
  activeProfile: string;
  profileTargetLufs: number;
  selectedFile: string;
  detailText: string;
  dashboardText: string;
  auditText: string;
  aboutText: string;
  profiles: string[];
}

// Navigate to the app and wait until the (Qt) UI has initialized and published
// its first state. The WASM module download + instantiation can be slow on a
// cold CI runner, hence the generous timeout.
export async function gotoApp(page: Page): Promise<void> {
  await page.goto('/index.html');
  await page.waitForFunction(
    () => !!(window as any).__djState && (window as any).__djState.ready === true,
    undefined,
    { timeout: 45_000 },
  );
}

// Queue a UI command for the bridge pump to consume.
export async function sendCmd(page: Page, cmd: string): Promise<void> {
  await page.evaluate((c) => {
    (window as any).__djCmd = c;
  }, cmd);
}

// Snapshot the current published state.
export async function getState(page: Page): Promise<DjState> {
  return await page.evaluate(() => (window as any).__djState as DjState);
}

// Poll a single field of the published state until it satisfies a matcher.
export function pollState<K extends keyof DjState>(page: Page, key: K) {
  return expect.poll(async () => (await getState(page))[key], {
    timeout: 30_000,
  });
}
