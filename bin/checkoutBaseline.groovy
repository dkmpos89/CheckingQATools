import com.serena.dmclient.api.DimensionsConnectionManager
import com.serena.dmclient.api.DimensionsConnectionDetails
import com.serena.dmclient.api.DimensionsRelatedObject
import com.serena.dmclient.api.DimensionsDatabaseAdmin
import com.serena.dmclient.api.DimensionsConnection
import com.serena.dmclient.collections.Products
import com.serena.dmclient.api.SystemAttributes
import com.serena.dmclient.api.BulkOperator
import com.serena.dmclient.api.ItemRevision
import com.serena.dmclient.objects.Product
import com.serena.dmclient.api.Baseline
import com.serena.dmclient.api.Filter
import com.serena.dmclient.api.Request
import com.serena.dmclient.api.Project

import es.als.io.IOUtils


public class checkoutBaseline {

	private static String msgEr = "<strong><span style='color:red;margin-top:0px; margin-bottom:0px; margin-left:80px; margin-right:0px;'>[ ERROR ] : </span></strong>"
	private static String msgOk = "<strong><span style='color:green;margin-top:0px; margin-bottom:0px; margin-left:80px; margin-right:0px;'>[ CORRECTO ] : </span></strong>"
	private static String br_object = "" 						// linea base o request
	private static List<String> lista_items = []


	private static String username = "dmsys";
	private static String password = "BcoRipley12";
	private static String dbname = "dimensions_adm";
	private static String dbconn ="DIM12";
	private static String ip = "192.168.10.66:48654";
	private static DimensionsConnection connection = null
	
	
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
		String repoPath = System.getProperty("user.dir")+"\\reportes"

		br_object = args[0]

		show("## CONECTANDO CON DIMENSIONS:")
	
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

			show("## BUSCANDO EL BASELINE:")
			//filtro para identificar el baseline objetivo de la descarga
			Filter filter = new Filter()
			filter.criteria().add(new Filter.Criterion(SystemAttributes.OBJECT_ID , br_object, Filter.Criterion.EQUALS))
			List<Baseline> baselines = connection.getObjectFactory().getBaselines(filter)

			//solo deberia devolver un baseline 
			if (baselines.size() == 0)
				failed("${msgEr} BASELINE: ${br_object} NO ENCONTRADO")
			else if(baselines.size() == 1)
				show("${msgOk} BASELINE ENCONTRADOS: ${baselines.size()}") //imprimirBaselineInfo(baselines)
			else
				failed("${msgEr} MAS DE UN BASELINE NO PERMITIDO A LA VEZ")

			// Busqueda y seteo del Proyecto por defecto 
			show("## BUSCANDO PROYECTOS:")
			Project dmproject = getProjectFromBaseline(baselines)
			connection.getObjectFactory().setCurrentProject(dmproject.getName(), false, ".", null, null, true)

			show("## BUSCANDO COMPONENTES EN LINEA BASE:")
			filter = new Filter()
			filter.criteria().add(new Filter.Criterion(SystemAttributes.IS_LATEST_REV))
			List listRItem = baselines.get(0).getChildItems(filter)
			
			if (listRItem.size() == 0)
				failed("${msgEr} LINEA BASE ${br_object} VACIA : ${listRItem.size()}")
			else
				show("${msgOk} ITEMS ENCONTRADOS: ${listRItem.size()}")
			

			/* Bulk operator: Consulta los atributos a todos los items de la lista de una sola vez */
		    BulkOperator bulk = connection.getObjectFactory().getBulkOperator(listRItem)
		    int[] attrs = [ SystemAttributes.OBJECT_UID, SystemAttributes.ITEMFILE_DIR, SystemAttributes.REVISION, SystemAttributes.ITEMFILE_FILENAME, SystemAttributes.FULL_PATH_NAME ]
		    bulk.queryAttribute(attrs)

			/* Stages de lineas bases a considerar */
		    def stages = [ "TEST_DESARROLLO", "QA", "PRE-PRODUCCION", "PRODUCCION" ]
			show("## INICIO")
			
			listRItem.each {
				ItemRevision item = (ItemRevision) it.getObject()
				print((String) item.getAttribute(SystemAttributes.ITEMFILE_FILENAME)+"/")

				//Se agregan a  lista de verificacion posterior
				lista_items << (String) item.getAttribute(SystemAttributes.FULL_PATH_NAME )

				def filtro = new Filter()
	            filtro.criteria().add(new Filter.Criterion(SystemAttributes.TYPE_NAME, "BL_RELEASE", Filter.Criterion.EQUALS))
	            filtro.criteria().add(new Filter.Criterion(SystemAttributes.STAGE, stages, Filter.Criterion.LESS_EQUAL ))
	            List<Baseline> all_baselines = item.getChildBaselines(filtro)
	            
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

			imprimirBaselineInfo(baselines, lista_items)
		}
		finally {
			// desconexion Dimensions			
			if (connection != null) connection.close()
		}
	}

// #####################################################################################################################

	static void imprimirBaselineInfo(List<Baseline> lista_baselines, List<String> items)
	{
		def baseline = lista_baselines.get(0)
		String pattern = /^.*\s+.*$/

        def matcher = baseline.getName() =~ pattern
        matcher.each {
            show("${msgEr} NOMBRE DE LA LINEA BASE CON ESPACIOS: ${it}")
        }

        items.each { item->
        	matcher = item =~ pattern
        	matcher.each {
            	show("${msgEr} RUTA O NOMBRE DE COMPONENTE CON ESPACIOS: ${it}")
            }
        }

        matcher = null

	}

	static Project findProject(DimensionsConnection connection, String productID, String projectID) 
	{
        DimensionsDatabaseAdmin defaultDatabase = connection.getObjectFactory().getBaseDatabaseAdmin();
        Products products = defaultDatabase.getProducts();
        Product product = products.get(productID);
        Filter filter = new Filter();
        filter.criteria().add(
                new Filter.Criterion(SystemAttributes.OBJECT_ID, projectID, Filter.Criterion.EQUALS));
        List projects = product.getProjects(filter);
        return (Project) projects.get(0);
    }

    static Project getProjectFromBaseline(List<Baseline> lista_baselines) 
    {
    	def lista_projects = lista_baselines.get(0).getParentProjects()
    	lista_projects.each {
    		show("${msgOk} ${it.getObject()}")
    	}
    	
        Project dmproject = (Project) lista_projects.get(0).getObject()
        return dmproject
    }
}
