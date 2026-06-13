import { test, expect } from '@playwright/test';
import fs from 'fs';
import { createCtfDatasource, resetDatasources } from '../helpers/api';
import { TEST_CTF_PATH } from '../helpers/constants';

test.describe('Timeline UI (CTF)', () => {
  test.beforeAll(async ({ request }) => {
    test.skip(!fs.existsSync(TEST_CTF_PATH), `missing test ctf: ${TEST_CTF_PATH}`);
    await resetDatasources(request);
    const ds = await createCtfDatasource(request);
    expect(ds.ingest_status).toBe('ready');
  });

  test('renders span swimlanes in timeline view', async ({ page }) => {
    await page.goto('/explorer');
    await expect(page.getByText('数据探索')).toBeVisible();

    // CTF span 表预设，标签为表短名
    await page.getByRole('button', { name: 'ctf_spans', exact: true }).click();
    await page.getByRole('button', { name: /查\s*询/ }).click();

    // span 数据应优先选中 Timeline 可视化
    await expect(page.getByRole('tab', { name: /Timeline/ })).toBeVisible({ timeout: 20_000 });
    await expect(page.locator('canvas')).toBeVisible();
    await expect(page.getByRole('button', { name: '重置缩放' })).toBeVisible();
    await expect(page.getByText(/\d+ 泳道 \/ \d+ 区间/)).toBeVisible();
  });
});
