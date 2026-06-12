import { test, expect } from '@playwright/test';
import fs from 'fs';
import { seedReadyDatasource } from '../helpers/api';
import { TEST_MCAP_PATH } from '../helpers/constants';

test.describe('Datasource UI', () => {
  test.beforeAll(async ({ request }) => {
    test.skip(!fs.existsSync(TEST_MCAP_PATH), `missing test mcap: ${TEST_MCAP_PATH}`);
    await seedReadyDatasource(request);
  });

  test('lists ros2 datasource with ready ingest status', async ({ page }) => {
    await page.goto('/datasources');
    await expect(page.getByText('数据源管理')).toBeVisible();
    await expect(page.getByText('e2e-test')).toBeVisible();
    await expect(page.getByText('ready').first()).toBeVisible();
    await expect(page.getByText('ros2_mcap')).toBeVisible();
  });

  test('navigates between explorer and datasource pages', async ({ page }) => {
    await page.goto('/explorer');
    await page.getByRole('link', { name: '数据源' }).click();
    await expect(page).toHaveURL(/\/datasources$/);
    await page.getByRole('link', { name: '指标探索' }).click();
    await expect(page).toHaveURL(/\/explorer$/);
  });
});
