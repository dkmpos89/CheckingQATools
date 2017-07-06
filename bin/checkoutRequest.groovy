import com.serena.dmclient.api.DimensionsConnectionManager
import com.serena.dmclient.api.DimensionsConnectionDetails
import com.serena.dmclient.api.DimensionsRelatedObject
import com.serena.dmclient.api.DimensionsDatabaseAdmin
import com.serena.dmclient.api.DimensionsConnection
import com.serena.dmclient.api.SystemAttributes
import com.serena.dmclient.collections.Products
import com.serena.dmclient.api.ItemRevision
import com.serena.dmclient.api.BulkOperator
import com.serena.dmclient.objects.Product
import com.serena.dmclient.api.Request
import com.serena.dmclient.api.Project
import com.serena.dmclient.api.Filter
import com.serena.dmclient.api.Baseline
import es.als.io.IOUtils


public class checkoutBaseline {

	private static String msgEr = "<strong><span style='color:red;margin-top:0px; margin-bottom:0px; margin-left:80px; margin-right:0px;'>[ ERROR ] : </span></strong>"
	private static String msgOk = "<strong><span style='color:green;margin-top:0px; margin-bottom:0px; margin-left:80px; margin-right:0px;'>[ CORRECTO ] : </span></strong>"
	private static String msgInf = "<strong><span style='color:#FAEB0F;margin-top:0px; margin-bottom:0px; margin-left:80px; margin-right:0px;'>[ DETALLE ] : </span></strong>"
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
		println(message)
	}
	static void failed(String message){
		println(message)
		println("\nFAIL")
		throw new Exception(message)
	}
	
	public static void main(String[] args) throws IOException 
	{
		String executionPath = System.getProperty("user.dir")
		List listItemReq = []
		//PropertyConfigurator.configure(executionPath+"/log4j.properties");
		product = args[0]
		project = args[1]
		br_object = args[2]
		atrib = args[3]

		show("## CONECTANDO CON DIMENSIONS: ")
		show("${msgInf} PRODUCTO: "+product)
		show("${msgInf} PROYECTO: "+project)
		show("${msgInf} OBJETO: "+br_object)
		
		try {
			// obteniendo la conexion con Dimensions
			
			DimensionsConnectionDetails details = new DimensionsConnectionDetails()
			details.setUsername(username)
			details.setPassword(password)
			details.setDbName(dbname)
			details.setDbConn(dbconn)
			details.setServer(ip)

			connection = DimensionsConnectionManager.getConnection(details)
			if(connection!=null) 
				show("${msgOk} CONECTANDO CON: ${ip}")
			else
				failed("${msgEr} NO SE PUEDE ESTABLECER LA CONEXION")
			
			
			show("## BUSCANDO PROYECTOS: ")
			// Busqueda y seteo del Proyecto por defecto 
			Project dmproject = findProject(connection, product, project)
			show("${msgOk} ${dmproject.getName()}")
			connection.getObjectFactory().setCurrentProject(dmproject.toString(), false, ".", null, null, true)

			show("## BUSCANDO COMPONENTES EN REQUEST ${br_object}: ")
			// Descarga de las ultimas revisiones de los items asociados
			Request pkgRequest = connection.getObjectFactory().findRequest(br_object)
			// Filtro para identificar el baseline objetivo de la descarga
			if (pkgRequest!= null && dmproject != null) {
				Filter filter = new Filter()
				filter.criteria().add(new Filter.Criterion(SystemAttributes.IS_LATEST_REV))
				listItemReq = pkgRequest.getChildItems(filter, dmproject)
			}else{
				failed("${msgEr} REQUEST: ${br_object} NO ENCONTRADO ")
			}

			if (listItemReq.size() == 0) 
				failed("${msgEr} REQUEST ${br_object} VACIO : "+listItemReq.size()+" ]")
			else
				show("${msgOk} ITEMS ENCONTRADOS: ${listItemReq.size()}" )

			if(listItemReq.size() >0 ) {
				/* Bulk operator: Consulta los atributos a todos los items de la lista de una sola vez */
			    BulkOperator bulk = connection.getObjectFactory().getBulkOperator(listItemReq)
			    int[] attrs = [ SystemAttributes.OBJECT_UID, SystemAttributes.STATUS, SystemAttributes.ITEMFILE_DIR, SystemAttributes.ITEMFILE_FILENAME ]
			    bulk.queryAttribute(attrs)

				def stages = [ "TEST_DESARROLLO", "QA", "PRE-PRODUCCION", "PRODUCCION" ]

				show("## INICIO")

				listItemReq.each { item->
					ItemRevision itemLastRevision  = (ItemRevision) item.getObject()
					print("\t"+(String) itemLastRevision.getAttribute(SystemAttributes.ITEMFILE_FILENAME)+"/")

					def filtro = new Filter()
		            filtro.criteria().add(new Filter.Criterion(SystemAttributes.TYPE_NAME, "BL_RELEASE", Filter.Criterion.EQUALS))
		            filtro.criteria().add(new Filter.Criterion(SystemAttributes.STAGE, stages, Filter.Criterion.LESS_EQUAL ))
		            List<Baseline> all_baselines = itemLastRevision.getChildBaselines(filtro)
		            
		            def baseline_stages = [:]
		            // Busqueda de informacion de la linea base
		            all_baselines.each { itembline ->
		                def cbline = (Baseline) itembline.getObject()
		                int[] queryAttr = [ SystemAttributes.STAGE, SystemAttributes.STATUS ]
		                cbline.queryAttribute(queryAttr)
		                List itemAttr = cbline.getAttribute(queryAttr)
		                baseline_stages.put("${itemAttr[0]}", 1)
		            }
		            print(baseline_stages.keySet())
		            print("\n")
				}

			show("## FIN")
			show("SUCCESS")
			}else
				failed("${msgEr} REQUEST VACIO ]")
		}
		finally {
			// desconexion Dimensions			
			if (connection != null) connection.close()
		}
	}

	static Project findProject(DimensionsConnection connection, String productID, String projectID) {
        DimensionsDatabaseAdmin defaultDatabase = connection.getObjectFactory().getBaseDatabaseAdmin();
        Products products = defaultDatabase.getProducts();
        Product product = products.get(productID);
        Filter filter = new Filter();
        filter.criteria().add(
                new Filter.Criterion(SystemAttributes.OBJECT_ID, projectID, Filter.Criterion.EQUALS));
        List projects = product.getProjects(filter);
        return (Project) projects.get(0);
    }
}
