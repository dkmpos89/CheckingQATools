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
void failed(String message)
{
	//err.println(message)
	//log.error(message)
	show(message);
	throw new Exception(message);
}

// Metodo de emision de error durante el script
static void show(String message)
{
	System.out.println(message);
}

// public static void main () {}
	show('Iniciando la descarga de items de Dimensions asociados a un baseline')
	String executionPath = System.getProperty("user.dir");

	File customPropertiesFile = new File(executionPath+"/plugin-custom.properties");
	if (!customPropertiesFile.exists()) 
		show("No se puede cargar fichero plugin-custom.properties con las propiedades de conexion de Dimensions");
	else
		show("Fichero plugin-custom.properties cargado exitosamente!");

	Properties customProperties = IOUtils.loadProperties(customPropertiesFile.newDataInputStream())
	
	String product = "COREBANCO";  // Producto a analizar
	String project = "BOTGEN";  // Proyecto a analizar 
	String br_object = "LB_BOTGEN_PRD_FNR_V1"; // linea base o request


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
  //log.info("Se va a proceder a conectar con Dimensions con los siguientes parametros: " + customProperties.username + ", " + customProperties.password + ", " + customProperties.dbname + ", " + customProperties.ip)
  show("Se va a proceder a conectar con Dimensions con los siguientes parametros: " + customProperties.username + ", " + customProperties.password + ", " + customProperties.dbname + ", " + customProperties.ip) 
  connection = DimensionsConnectionManager.getConnection(details);		
  //log.info("Conexion exitosa con Dimensions!!!");
  show("Conexion exitosa con Dimensions!!!");						
  
  //filtro para identificar el baseline objetivo de la descarga
  Filter filter = new Filter();
  filter.criteria().add(new Filter.Criterion(SystemAttributes.OBJECT_ID , br_object, Filter.Criterion.EQUALS));
  def baselines = connection.getObjectFactory().getBaselines(filter);
  
  //solo deberia devolver un baseline 
  if (baselines.size() == 0) failed('No se encontro el baseline');
  if (baselines.size() == 1) failed('Exito! baseline encontrado!');
  if (baselines.size() > 1) failed('Mas de un baseline no permitido a la vez');
  
  
}
finally {  
  // desconexion Dimensions			
  if (connection != null) connection.close();
}

