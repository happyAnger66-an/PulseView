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

    // 预设按钮由 schema 的 default_metrics 元数据生成，标签为表短名
    await page.getByRole('button', { name: 'system_stats', exact: true }).click();
    await page.getByRole('button', { name: /查\s*询/ }).click();

    await expect(page.locator('.uplot')).toBeVisible({ timeout: 20_000 });
    await expect(page.locator('.timeseries-stats-table')).toBeVisible();
    await expect(page.getByText('统计（全部数据）')).toBeVisible();
  });

  test('runs node sub hz preset and shows multi-series legend', async ({ page }) => {
    await page.goto('/explorer');
    await page.getByRole('button', { name: 'node_sub_stats', exact: true }).click();
    await page.getByRole('button', { name: /查\s*询/ }).click();

    await expect(page.locator('.uplot')).toBeVisible({ timeout: 20_000 });
    await expect(page.getByRole('button', { name: '全部显示' })).toBeVisible();
    await expect(page.getByRole('button', { name: '全部隐藏' })).toBeVisible();
    await expect(page.locator('.timeseries-legend-list li').first()).toBeVisible();
  });

  test('toggles legend visibility and updates stats table', async ({ page }) => {
    await page.goto('/explorer');
    await page.getByRole('button', { name: 'node_sub_stats', exact: true }).click();
    await page.getByRole('button', { name: /查\s*询/ }).click();

    // 只匹配数据行，排除 antd 的隐藏测量行（ant-table-measure-row）
    const dataRows = page.locator('.timeseries-stats-table tbody tr.ant-table-row');
    await expect(dataRows.first()).toBeVisible({ timeout: 20_000 });

    const rowsBefore = await dataRows.count();
    expect(rowsBefore).toBeGreaterThan(1);

    await page.getByRole('button', { name: '全部隐藏' }).click();
    await expect(dataRows).toHaveCount(0);

    await page.getByRole('button', { name: '全部显示' }).click();
    await expect(dataRows).toHaveCount(rowsBefore);
  });

  test('switches to raw table tab', async ({ page }) => {
    await page.goto('/explorer');
    await page.getByRole('button', { name: 'system_stats', exact: true }).click();
    await page.getByRole('button', { name: /查\s*询/ }).click();
    await expect(page.locator('.uplot')).toBeVisible({ timeout: 20_000 });

    await page.getByRole('tab', { name: /表格/ }).click();
    await expect(page.locator('.ant-table')).toBeVisible();
  });
});
