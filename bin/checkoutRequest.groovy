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
		List listItemReq;
		//PropertyConfigurator.configure(executionPath+"/log4j.properties");
		product = args[0];
		project = args[1];
		br_object = args[2];
		atrib = args[3];

		show("INICIANDO LA DESCARGA...");
		show("---> Producto: "+product);
		show("---> Proyecto: "+project);
		show("---> Objeto: "+br_object);

		
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
			  
			// Descarga de las ultimas revisiones de los items asociados
			Request pkgRequest = connection.getObjectFactory().findRequest(br_object);
			Project projectDimensions = connection.getObjectFactory().getProject(product + ':' + project)
			// Filtro para identificar el baseline objetivo de la descarga
			if (pkgRequest!= null && projectDimensions != null) {
				Filter filter = new Filter();
				filter.criteria().add(new Filter.Criterion(SystemAttributes.IS_LATEST_REV));
				listItemReq = pkgRequest.getChildItems(filter, projectDimensions);
			}else{
				failed("[ Request: "+br_object+" no encontrado ]");
			}


			//solo deberia devolver un baseline 
			if (listItemReq.size() == 0) 
				failed("[ Request "+br_object+" vacio : "+listItemReq.size()+" ]");
			else
				show("---> Items encontrados: "+listItemReq.size());

			if(listItemReq.size() >0 ){

				System.out.println("[INICIO]");
				for (DimensionsRelatedObject item : listItemReq) 
				{
					ItemRevision itemLastRevision  = (ItemRevision) item.getObject();
					int[] queryAttributes = [ SystemAttributes.OBJECT_UID, SystemAttributes.STATUS, SystemAttributes.ITEMFILE_DIR, SystemAttributes.ITEMFILE_FILENAME ];
					itemLastRevision.queryAttribute(queryAttributes);
					List attributes = itemLastRevision.getAttribute(queryAttributes);
					System.out.println(attributes.get(3).toString());
				}

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
