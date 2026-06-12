import { test, expect } from '@playwright/test';
import fs from 'fs';
import { seedReadyDatasource } from '../helpers/api';
import { TEST_MCAP_PATH } from '../helpers/constants';

test.describe('Explorer UI', () => {
  test.beforeAll(async ({ request }) => {
    test.skip(!fs.existsSync(TEST_MCAP_PATH), `missing test mcap: ${TEST_MCAP_PATH}`);
    await seedReadyDatasource(request);
  });

  test('loads explorer and runs cpu/memory preset query', async ({ page }) => {
    await page.goto('/explorer');
    await expect(page.getByText('数据探索')).toBeVisible();

    await page.getByRole('button', { name: 'CPU / 内存' }).click();
    await page.getByRole('button', { name: '查询' }).click();

    await expect(page.locator('.uplot')).toBeVisible({ timeout: 20_000 });
    await expect(page.locator('.timeseries-stats-table')).toBeVisible();
    await expect(page.getByText('统计（全部数据）')).toBeVisible();
  });

  test('runs node sub hz preset and shows multi-series legend', async ({ page }) => {
    await page.goto('/explorer');
    await page.getByRole('button', { name: 'Node Sub Hz' }).click();
    await page.getByRole('button', { name: '查询' }).click();

    await expect(page.locator('.uplot')).toBeVisible({ timeout: 20_000 });
    await expect(page.getByRole('button', { name: '全部显示' })).toBeVisible();
    await expect(page.getByRole('button', { name: '全部隐藏' })).toBeVisible();
    await expect(page.locator('.timeseries-legend-list li').first()).toBeVisible();
  });

  test('toggles legend visibility and updates stats table', async ({ page }) => {
    await page.goto('/explorer');
    await page.getByRole('button', { name: 'Node Sub Hz' }).click();
    await page.getByRole('button', { name: '查询' }).click();
    await expect(page.locator('.timeseries-stats-table tbody tr').first()).toBeVisible({
      timeout: 20_000,
    });

    const rowsBefore = await page.locator('.timeseries-stats-table tbody tr').count();
    expect(rowsBefore).toBeGreaterThan(1);

    await page.getByRole('button', { name: '全部隐藏' }).click();
    await expect(page.locator('.timeseries-stats-table tbody tr')).toHaveCount(0);

    await page.getByRole('button', { name: '全部显示' }).click();
    await expect(page.locator('.timeseries-stats-table tbody tr')).toHaveCount(rowsBefore);
  });

  test('switches to raw table tab', async ({ page }) => {
    await page.goto('/explorer');
    await page.getByRole('button', { name: 'CPU / 内存' }).click();
    await page.getByRole('button', { name: '查询' }).click();
    await expect(page.locator('.uplot')).toBeVisible({ timeout: 20_000 });

    await page.getByRole('tab', { name: /表格/ }).click();
    await expect(page.locator('.ant-table')).toBeVisible();
  });
});
