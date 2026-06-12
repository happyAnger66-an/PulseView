import { createContext, useContext, useEffect, useState, type ReactNode } from 'react';
import { Layout, Menu, Spin } from 'antd';
import { DatabaseOutlined, FundOutlined, LineChartOutlined } from '@ant-design/icons';
import { Link, useLocation } from 'react-router-dom';
import { getDatasources } from '@/services/datasource';
import type { Datasource } from '@/types';
import './style.less';

const { Header, Sider, Content } = Layout;

interface AppContextValue {
  datasources: Datasource[];
  reloadDatasources: () => Promise<void>;
  defaultDatasource?: Datasource;
}

export const AppContext = createContext<AppContextValue>({
  datasources: [],
  reloadDatasources: async () => {},
});

export function useAppContext() {
  return useContext(AppContext);
}

interface Props {
  children: ReactNode;
}

export default function AppLayout({ children }: Props) {
  const location = useLocation();
  const [loading, setLoading] = useState(true);
  const [datasources, setDatasources] = useState<Datasource[]>([]);

  const reloadDatasources = async () => {
    const list = await getDatasources();
    setDatasources(list);
  };

  useEffect(() => {
    reloadDatasources()
      .catch(() => setDatasources([]))
      .finally(() => setLoading(false));
  }, []);

  const defaultDatasource = datasources.find((d) => d.is_default) ?? datasources[0];

  const selectedKey = location.pathname.startsWith('/datasources')
    ? '/datasources'
    : location.pathname.startsWith('/explorer')
      ? '/explorer'
      : location.pathname;

  return (
    <AppContext.Provider value={{ datasources, reloadDatasources, defaultDatasource }}>
      <Layout className='app-layout'>
        <Sider width={200} theme='light' className='app-sider'>
          <div className='logo'>
            <FundOutlined className='logo-icon' />
            <span>PulseView</span>
          </div>
          <Menu mode='inline' selectedKeys={[selectedKey]}>
            <Menu.Item key='/explorer' icon={<LineChartOutlined />}>
              <Link to='/explorer'>指标探索</Link>
            </Menu.Item>
            <Menu.Item key='/datasources' icon={<DatabaseOutlined />}>
              <Link to='/datasources'>数据源</Link>
            </Menu.Item>
          </Menu>
        </Sider>
        <Layout>
          <Header className='app-header'>
            <span>监控前端 · SQLite/PromQL · ROS2 MCAP/DuckDB</span>
          </Header>
          <Content className='app-content'>
            {loading ? (
              <div className='page-loading'>
                <Spin />
              </div>
            ) : (
              children
            )}
          </Content>
        </Layout>
      </Layout>
    </AppContext.Provider>
  );
}
