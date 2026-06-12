import { Switch, Route, Redirect } from 'react-router-dom';
import ExplorerPage from '@/pages/explorer';
import DatasourcesPage from '@/pages/datasources';
import DatasourceFormPage from '@/pages/datasources/Form';

export default function AppRoutes() {
  return (
    <Switch>
      <Route path='/explorer' component={ExplorerPage} exact />
      <Route path='/datasources/add/:type' component={DatasourceFormPage} exact />
      <Route path='/datasources/edit/:type/:id' component={DatasourceFormPage} exact />
      <Route path='/datasources' component={DatasourcesPage} exact />
      <Redirect from='/' to='/explorer' exact />
    </Switch>
  );
}
