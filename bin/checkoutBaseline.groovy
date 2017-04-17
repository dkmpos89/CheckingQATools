import com.serena.dmclient.api.SystemAttributes;
import com.serena.dmclient.api.DimensionsConnectionManager;
import com.serena.dmclient.api.DimensionsConnection;
import com.serena.dmclient.api.DimensionsConnectionDetails;
import com.serena.dmclient.api.Filter;
import com.serena.dmclient.api.DimensionsRelatedObject;
import com.serena.dmclient.api.ItemRevision;

import es.als.io.IOUtils;
import com.serena.dmclient.api.Request;
import com.serena.dmclient.api.Project;
import com.serena.dmclient.api.Baseline;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;

public class checkoutBaseline {

	//private static Logger log = Logger.getLogger(checkoutBaseline.class);
	private static String product = "COREBANCO";  						// Producto a analizar
	private static String project = "INTEGRACION_DE_SERVICIOS";  		// Proyecto a analizar 
	private static String br_object = "LB_INI_B060A_20170404"; 			// linea base o request
	private static String username = "dmsys";
	private static String password = "BcoRipley12";
	private static String dbname = "dimensions_adm";
	private static String dbconn ="DIM12";
	private static String ip = "192.168.10.66:48654";
	private static DimensionsConnection connection = null;
	
	
	// Metodo de emision de error durante el script
	static void show(String message){
		System.out.println(message);
	}
	
	public static void main(String[] args) throws IOException 
	{
		String executionPath = System.getProperty("user.dir");
		//PropertyConfigurator.configure(executionPath+"/log4j.properties");
		
		show("Iniciando la descarga de items de Dimensions asociados a un baseline");

		// Validando acceso a properties con datos de conexión
		File customPropertiesFile = new File(executionPath+"/plugin-custom.properties");
		if (!customPropertiesFile.exists()) 
			show("No se puede cargar fichero plugin-custom.properties con las propiedades de conexion de Dimensions");
		else
			show("Fichero plugin-custom.properties cargado exitosamente!");
		
		//InputStream fileStream = new FileInputStream(customPropertiesFile);
		//customPropertiesFile.load(fileStream);
		//Properties customProperties = IOUtils.loadProperties(fileStream);

		if (project == null) 
			show("No existe project seleccionado. Por favor, especificar un proyecto para la descarga de items asociados al baseline");
		else
			show("Proyecto: "+project+" cargado exitosamente!");
		
		try {
			// obteniendo la conexion con Dimensions
			
			DimensionsConnectionDetails details = new DimensionsConnectionDetails();
			details.setUsername(username);
			details.setPassword(password);
			details.setDbName(dbname);
			details.setDbConn(dbconn);
			details.setServer(ip);
			show("Se va a proceder a conectar con Dimensions con los siguientes parametros: " + username + ", " + password + ", " + dbname + ", " + ip);
 
			connection = DimensionsConnectionManager.getConnection(details);		
			if(connection!=null) 
				show("Conexion exitosa con Dimensions.");
			else
				show("No se puede conectar con Dimensions.");
			  
			//filtro para identificar el baseline objetivo de la descarga
			Filter filter = new Filter();
			filter.criteria().add(new Filter.Criterion(SystemAttributes.OBJECT_ID , br_object, Filter.Criterion.EQUALS));
			List<Baseline> baselines = connection.getObjectFactory().getBaselines(filter);
			  
			//solo deberia devolver un baseline 
			if (baselines.size() == 0) 
				show("Linea base no encontrada: "+baselines.size());
			else if(baselines.size() == 1){
				show("Baseline encontrado: "+baselines.size());
			}else
				show("Mas de un baseline no permitido a la vez");

			filter = new Filter();
			filter.criteria().add(new Filter.Criterion(SystemAttributes.IS_LATEST_REV));
			List<DimensionsRelatedObject> listItemRelationships = baselines.get(0).getChildItems(filter);
			  
			show("Tamaño de lista de items: "+listItemRelationships.size());
			  
			System.out.println("\nLista de items: ");
			for (DimensionsRelatedObject item : listItemRelationships) 
			{
				ItemRevision itemLastRevision  = (ItemRevision) item.getObject();
				int[] queryAttributes = [ SystemAttributes.OBJECT_UID, SystemAttributes.STATUS, SystemAttributes.ITEMFILE_DIR, SystemAttributes.ITEMFILE_FILENAME ];
				itemLastRevision.queryAttribute(queryAttributes);
				List attributes = itemLastRevision.getAttribute(queryAttributes);
				System.out.println(attributes.get(3).toString());
				
			}	  
		}
		finally {  
		  // desconexion Dimensions			
		  if (connection != null) connection.close();
		}
	}
}
