import com.serena.dmclient.api.SystemAttributes;
import com.serena.dmclient.api.DimensionsConnectionManager;
import com.serena.dmclient.api.DimensionsConnection;
import com.serena.dmclient.api.DimensionsConnectionDetails;
import com.serena.dmclient.api.Filter;
import com.serena.dmclient.api.DownloadCommandDetails;
import com.serena.dmclient.api.DimensionsResult;
import com.serena.dmclient.api.ItemRevision;

import es.als.checking.framework.util.AntUtils;
import es.als.io.IOUtils;
import com.als.checking.model.project.ProjectHelper

import groovy.xml.MarkupBuilder;


//metodo de emision de error durante el script
void failed(String message){
  err.println(message)
  log.error(message)
  throw new Exception(message);
}

//main
out.println('Iniciando la descarga de items de Dimensions asociados a un baseline')

//validando acceso a properties con datos de conexion
File customPropertiesFile = new File(System.getenv("CHECKING_DATA"), "config/plugins/GLOBAL/plugin-custom.properties")
if (!customPropertiesFile.exists()) failed('No se puede cargar fichero plugin-custom.properties con las propiedades de conexion de Dimensions')
  
Properties customProperties = IOUtils.loadProperties(customPropertiesFile.newDataInputStream())

if (project == null) failed('No existe project seleccionado. Por favor, especificar un proyecto para la descarga de items asociados al baseline')

DimensionsConnection connection = null;
try {
  
  // obteniendo la conexion con Dimensions  
  DimensionsConnectionDetails details = new DimensionsConnectionDetails();
  details.setUsername(customProperties.username);
  details.setPassword(customProperties.password);
  details.setDbName(customProperties.dbname);
  details.setDbConn(customProperties.dbconn);
  details.setServer(customProperties.ip);
  log.info("Se va a proceder a conectar con Dimensions con los siguientes parametros: " + customProperties.username + ", " + customProperties.password + ", " + customProperties.dbname + ", " + customProperties.ip)
  out.println("Se va a proceder a conectar con Dimensions con los siguientes parametros: " + customProperties.username + ", " + customProperties.password + ", " + customProperties.dbname + ", " + customProperties.ip) 
  connection = DimensionsConnectionManager.getConnection(details);		
  log.info("Conexion exitosa con Dimensions!!!");
  out.println("Conexion exitosa con Dimensions!!!");						
  
  File softwarePath = ProjectHelper.getPath(software) 
  // borrado de la ruta del proyecto previa
  AntUtils.rmdir(softwarePath);
  
  // Creando la ruta nueva ruta del proyecto
  softwarePath.mkdir();
 

  //filtro para identificar el baseline objetivo de la descarga
  Filter filter = new Filter();
  filter.criteria().add(new Filter.Criterion(SystemAttributes.OBJECT_ID , parameters.baseline_name, Filter.Criterion.EQUALS));
  def baselines = connection.getObjectFactory().getBaselines(filter);
  
  //solo deberia devolver un baseline 
  if (baselines.size() != 1) failed('Mas de un baseline no permitido a la vez')
  
  //descarga de los items del baseline
  DownloadCommandDetails commandDetails = new DownloadCommandDetails();		
  commandDetails.setRecursive(true);
  commandDetails.setUserDirectory(softwarePath.getPath());
  commandDetails.setUpdate(true);
  commandDetails.setOverwrite(true);
  commandDetails.setMetadata(false);
  DimensionsResult result = baselines[0].download(commandDetails);
  log.info("DimensionsResult: " + result.getMessage());
  out.println("DimensionsResult: " + result.getMessage());
  
  //generacion del inventario XML
  Map inventoryItems = [:]
  filter = new Filter();
  filter.criteria().add(new Filter.Criterion(SystemAttributes.IS_LATEST_REV));
  List listItemRelationships = baselines[0].getChildItems(filter);
  listItemRelationships.each{ item ->
    ItemRevision itemLastRevision  = (ItemRevision) item.getObject();
    int[] queryAttributes = [SystemAttributes.OBJECT_UID,SystemAttributes.STATUS,SystemAttributes.ITEMFILE_DIR,SystemAttributes.ITEMFILE_FILENAME]
    itemLastRevision.queryAttribute(queryAttributes);
    List attributes = itemLastRevision.getAttribute(queryAttributes)
    //en el caso de que el ITEMFILE_DIR sea el raiz, el API de Dimensions devuelve null
    if (attributes[2] == null) attributes[2] = ''
    inventoryItems[itemLastRevision.getName()] = ['uid':attributes[0],'status':attributes[1],'path': es.als.util.XMLUtils.escape(attributes[2] + attributes[3])]		
  }
  //construccion del XML
  out.println("Depurando la lista de items: " + inventoryItems)
  evaluate(new File(System.getenv("CHECKING_DATA") + '/config/plugins/GLOBAL/xmlGeneration.groovy'))
  def xmlGeneration = new xmlGeneration()
  xmlGeneration.xmlGeneration(project,inventoryItems)
  
}
finally {  
  // desconexion Dimensions			
  if (connection != null) connection.close();
}

