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


public class checkoutBaseline {

	private static String product = "";  						// Producto a analizar
	private static String project = "";  						// Proyecto a analizar 
	private static String br_object = ""; 						// linea base o request
	private static String atrib = ""; 							// Atributo proyecto

	private static String myItemslist = ""; 

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
	static void failed(String message){
		System.out.println(message);
		show("\nPROCESO-TERMINADO-FAIL");
		throw new Exception(message);
	}
	
	public static void main(String[] args) throws IOException 
	{
		String executionPath = System.getProperty("user.dir");
		//PropertyConfigurator.configure(executionPath+"/log4j.properties");
		product = args[0];
		project = args[1];
		br_object = args[2];
		atrib = args[3];

		show("INICIANDO LA DESCARGA...");
		show("---> Producto: "+product);
		show("---> Proyecto: "+project);
		show("---> Objeto: "+br_object);

		// Validando acceso a properties con datos de conexión
		//File customPropertiesFile = new File(executionPath+"/plugin-custom.properties");
		//if (!customPropertiesFile.exists()) 
			//show("No se puede cargar fichero plugin-custom.properties con las propiedades de conexion de Dimensions");
		//else
			//show("Fichero plugin-custom.properties cargado exitosamente!");
		
		//InputStream fileStream = new FileInputStream(customPropertiesFile);
		//customPropertiesFile.load(fileStream);
		//Properties customProperties = IOUtils.loadProperties(fileStream);

		//if (project == null) 
			//show("No existe project seleccionado. Por favor, especificar un proyecto para la descarga de items asociados al baseline");
		//else
			//show("Proyecto: "+project+" cargado exitosamente!");
		
		try {
			// obteniendo la conexion con Dimensions
			
			DimensionsConnectionDetails details = new DimensionsConnectionDetails();
			details.setUsername(username);
			details.setPassword(password);
			details.setDbName(dbname);
			details.setDbConn(dbconn);
			details.setServer(ip);
			show("Conectando con Dimensions...");
 
			connection = DimensionsConnectionManager.getConnection(details);		
			if(connection!=null) 
				show("---> Conexion exitosa");
			else{
				failed("[ No se puede conectar con Dimensions ]");
			}
			  
			//filtro para identificar el baseline objetivo de la descarga
			Filter filter = new Filter();
			filter.criteria().add(new Filter.Criterion(SystemAttributes.OBJECT_ID , br_object, Filter.Criterion.EQUALS));
			List<Baseline> baselines = connection.getObjectFactory().getBaselines(filter);
			  
			//solo deberia devolver un baseline 
			if (baselines.size() == 0) 
				failed("[ Baseline: "+br_object+" no encontrada - "+baselines.size()+" ]");
			else if(baselines.size() == 1)
			{
				show("---> Baseline encontrados: "+baselines.size());
			}else
				failed("[ Mas de un baseline no permitido a la vez ]");

			filter = new Filter();
			filter.criteria().add(new Filter.Criterion(SystemAttributes.IS_LATEST_REV));
			List<DimensionsRelatedObject> listItemRelationships = baselines.get(0).getChildItems(filter);
			  
			show("---> Items encontrados: "+listItemRelationships.size());
			if(listItemRelationships.size() >0 ){

				System.out.println("[INICIO]");
				for (DimensionsRelatedObject item : listItemRelationships) 
				{
					ItemRevision itemLastRevision  = (ItemRevision) item.getObject();
					int[] queryAttributes = [ SystemAttributes.OBJECT_UID, SystemAttributes.STATUS, SystemAttributes.ITEMFILE_DIR, SystemAttributes.ITEMFILE_FILENAME ];
					itemLastRevision.queryAttribute(queryAttributes);
					List attributes = itemLastRevision.getAttribute(queryAttributes);
					System.out.println(attributes.get(3).toString());
					//myItemslist = myItemslist+attributes.get(3).toString()+";";
				}
/*				def cmd = 'cmd /c setx LIST_OF_ITEMS /M '+myItemslist;
				//println(cmd)
				def out = new StringBuffer()
				def err = new StringBuffer()
				def proc = cmd.execute()
				proc.consumeProcessOutput(out,err)
				proc.waitForOrKill(60000)

				if(proc.exitValue() != 0)
					failed("Error de ejecucion en proceso 'setx'")
				else
					println err.size()==0 ? "OK" : "OK, pero con errores de ejecucion!"
*/
				System.out.println("[FIN]");
				show("PROCESO-TERMINADO-SUCCESS");
			}else
				failed("[ ERROR - Linea Base vacia ]");
		}
		finally {
			// desconexion Dimensions			
			if (connection != null) connection.close();
		}
	}
}
