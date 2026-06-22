import { test, expect } from '@playwright/test';
import { gotoApp, sendCmd, getState, pollState } from './helpers';

// End-to-end coverage of the major user flows, driven through the WASM test
// bridge (window.__djState / window.__djCmd). Each test starts from a fresh
// page, i.e. a fresh in-memory app instance.
test.beforeEach(async ({ page }) => {
  await gotoApp(page);
});

test('app loads with an empty library and sensible defaults', async ({ page }) => {
  const s = await getState(page);
  expect(s.windowTitle).toBe('DJ Library Goodizer');
  expect(s.version).toMatch(/^\d+\.\d+\.\d+$/);
  expect(s.rowCount).toBe(0);
  expect(s.currentTab).toBe(0);
  expect(s.tabName).toBe('Library');
  expect(s.statusText).toContain('core v');
  expect(s.activeProfile).toBe('Club / USB');
  expect(s.profiles).toEqual([
    'Club / USB',
    'Mono Broadcast',
    'Archival / Lossless',
  ]);
});

test('Load Demo Library populates the library table', async ({ page }) => {
  await sendCmd(page, 'loadDemo');
  await pollState(page, 'rowCount').toBe(8);

  const s = await getState(page);
  expect(s.statusText).toContain('Loaded demo library');
  expect(s.currentTab).toBe(0);
});

test('selecting a track shows its analysis metrics', async ({ page }) => {
  await sendCmd(page, 'loadDemo');
  await pollState(page, 'rowCount').toBe(8);

  await sendCmd(page, 'selectTrack:0');
  await pollState(page, 'selectedFile').toBe('01 Opening Set.flac');

  const s = await getState(page);
  expect(s.detailText).toContain('Integrated loudness');
  expect(s.detailText).toContain('LUFS');
});

test('dashboard summarizes the loaded library', async ({ page }) => {
  await sendCmd(page, 'loadDemo');
  await pollState(page, 'rowCount').toBe(8);

  await sendCmd(page, 'setTab:1');
  await pollState(page, 'currentTab').toBe(1);

  const s = await getState(page);
  expect(s.tabName).toBe('Dashboard');
  expect(s.dashboardText).toContain('Tracks: 8 (8 analyzed)');
  expect(s.dashboardText).toContain('Loudness range:');
  expect(s.dashboardText).toContain('Active profile: Club / USB');
});

test('audit tab reports no processing yet', async ({ page }) => {
  await sendCmd(page, 'setTab:2');
  await pollState(page, 'currentTab').toBe(2);

  const s = await getState(page);
  expect(s.tabName).toBe('Audit Log');
  expect(s.auditText).toContain('No processing');
});

test('switching the active profile updates target and status', async ({ page }) => {
  await sendCmd(page, 'loadDemo');
  await pollState(page, 'rowCount').toBe(8);

  await sendCmd(page, 'setProfile:1');
  await pollState(page, 'activeProfile').toBe('Mono Broadcast');

  const s = await getState(page);
  expect(s.profileTargetLufs).toBeCloseTo(-16.0, 1);
  expect(s.statusText).toContain('Active profile: Mono Broadcast');

  // The dashboard reflects the newly active profile.
  await sendCmd(page, 'setTab:1');
  await pollState(page, 'currentTab').toBe(1);
  expect((await getState(page)).dashboardText).toContain('Mono Broadcast');
});

test('loading a file from disk imports it via the real decode pipeline', async ({ page }) => {
  // loadDiskSample writes a real WAV into the (virtual) filesystem and imports
  // it through openDecoder→analyze→persist — the same path the folder picker
  // uses for user-selected files.
  await sendCmd(page, 'loadDiskSample');
  await pollState(page, 'rowCount').toBe(1);

  const s = await getState(page);
  expect(s.statusText).toContain('from disk');
});

test('Standardize Library produces a dry-run plan in the audit log', async ({ page }) => {
  await sendCmd(page, 'loadDemo');
  await pollState(page, 'rowCount').toBe(8);

  await sendCmd(page, 'standardize');
  await pollState(page, 'currentTab').toBe(2);

  const s = await getState(page);
  expect(s.tabName).toBe('Audit Log');
  expect(s.auditText).toContain('Standardization plan');
  expect(s.statusText).toContain('Standardized 8 track(s)');
});

test('the folder picker hook is exposed for the File menu action', async ({ page }) => {
  const hookType = await page.evaluate(() => typeof (window as any).__djPickFolder);
  expect(hookType).toBe('function');
});

test('About surfaces the application description', async ({ page }) => {
  await sendCmd(page, 'about');
  await pollState(page, 'aboutText').toContain('DJ Library Preparation');

  expect((await getState(page)).aboutText).toContain('Core v');
});
