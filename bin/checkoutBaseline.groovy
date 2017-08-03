import com.serena.dmclient.api.DimensionsConnectionManager
import com.serena.dmclient.api.DimensionsConnectionDetails
import com.serena.dmclient.api.DimensionsRelatedObject
import com.serena.dmclient.api.DimensionsDatabaseAdmin
import com.serena.dmclient.api.DownloadCommandDetails
import com.serena.dmclient.api.DimensionsConnection
import com.serena.dmclient.collections.Products
import com.serena.dmclient.api.SystemAttributes
import com.serena.dmclient.api.DimensionsResult
import com.serena.dmclient.api.BulkOperator
import com.serena.dmclient.api.ItemRevision
import com.serena.dmclient.objects.Product
import com.serena.dmclient.api.Baseline
import com.serena.dmclient.api.Filter
import com.serena.dmclient.api.Request
import com.serena.dmclient.api.Project

import es.als.io.IOUtils


public class checkoutBaseline {

	private static String msgEr = "<strong><span style='color:red;margin-left:80px; margin-right:0px;'>[ ERROR ] : </span></strong>"
	private static String msgOk = "<strong><span style='color:green;margin-left:80px; margin-right:0px;'>[ CORRECTO ] : </span></strong>"
	private static String msgInf = "<strong><span style='color:#FAEB0F;margin-left:80px; margin-right:0px;'>[ DETALLES ] : </span></strong>"
	private static String br_object = "" 						// linea base o request
	private static String br_ambiente = ""
	private static String br_download = ""
	private static List<String> lista_items = []
    private static Properties propsDimensions = new Properties()
	private static DimensionsConnection connection = null
	private static String executionPath = ""
	private static String cPath = ""
	
	// Metodo de emision de error durante el script
	static void show(String message){
		println(message)
	}
	static void failed(String message){
		println(message)
		println("\nFAIL!")
		throw new Exception(message)
	}
	
	public static void main(String[] args) throws IOException 
	{
		executionPath = System.getProperty("user.dir")
		cPath = System.getProperty("user.dir")-"\\bin";


/*
		Properties p = System.getProperties();
      	p.list(System.out);
*/
		br_object = args[0]
		br_ambiente = args[1]
		br_download = args[2]

		show("## CONECTANDO CON DIMENSIONS:")
	
		try {
 			loadProperties()	
			connection = crearConexion()		
			if(connection!=null)
				show("${msgOk} CONECTANDO CON: "+propsDimensions['ip'])
			else
				failed("${msgEr} NO SE PUEDE ESTABLECER LA CONEXION")

			//"## BUSCANDO EL BASELINE
			//filtro para identificar el baseline objetivo de la descarga
			Filter filter = new Filter()
			filter.criteria().add(new Filter.Criterion(SystemAttributes.OBJECT_ID , br_object, Filter.Criterion.EQUALS))
			List<Baseline> baselines = connection.getObjectFactory().getBaselines(filter)
			//solo deberia devolver un baseline 
			if (baselines.size() == 0)
				failed("${msgEr} BASELINE: ${br_object} NO ENCONTRADO")
			else if(baselines.size() == 1)
			{	
				// Busqueda y seteo del Proyecto por defecto 
				show("## BUSCANDO PROYECTOS:")
				Project dmproject = getProjectFromBaseline(baselines)
				String[] detalle = dmproject.getName().split(":")
				String product = detalle[0]
				String project = detalle[1]

				switch (br_ambiente) {
					case ~/^DESA$/: show("${msgOk} ${dmproject.getName()}"); break;

					case ~/^PRE$/:  dmproject = findProject(connection,product,"${project}_PRE");
									show("${msgOk} ${dmproject.getName()}");
									break;

					case ~/^PROD$/: dmproject = findProject(connection,product,"PRODUCCION_RIPLEY");
									show("${msgOk} ${dmproject.getName()}");
									break;

					default: failed("${msgEr} ANALISIS EN AMBIENTE NO DEFINIDO!");
				}

				connection.getObjectFactory().setCurrentProject(dmproject.getName(), false, ".", null, null, true)

				show("## BUSCANDO EL BASELINE:")
				int[] queryAttr = [ SystemAttributes.STAGE ]
				def cBaseline = baselines.get(0)
                cBaseline.queryAttribute(queryAttr)
                List itemAttr = cBaseline.getAttribute(queryAttr)
				show("${msgOk} ${cBaseline.getName()} - STAGE: [ ${itemAttr[0]} ]")
			}
			else
				failed("${msgEr} MAS DE UN BASELINE NO PERMITIDO A LA VEZ")

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
			show("## INICIO-COMPONENTES")
			
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

			show("## FIN-COMPONENTES")
			show("SUCCESS!")

			imprimirBaselineInfo(baselines, lista_items)
			if(br_download.equals("true"))
				analisisProperties("${cPath}\\reportes\\${br_object}")

			Thread.sleep(1000)
			show("DONE!")

		}catch(Exception e){
			show("${msgEr} ${e.toString()}")
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
		/* 1. Analisis en busca de errores en los componentes de la linea base */
		String patternChars = /^.*(\s+|\(+|\)|\{+|\}+)+.*$/
        def matcher = baseline.getName() =~ patternChars
        matcher.each {
            show("${msgEr} Nombre de la linea base con caracter peligroso:    ${it}")
        }

        items.each { item->
            matcher = item =~ patternChars
        	matcher.each {
            	show("${msgEr} Componente con caracteres especiales:    ${it}")
            }
        }

        /* 2. Descarga de los componentes de la linea base y busqueda de properties */
		DownloadCommandDetails commandDetails = new DownloadCommandDetails()
		commandDetails.setRecursive(true)
		commandDetails.setUserDirectory("${cPath}\\reportes\\${br_object}")
		commandDetails.setUpdate(true)
		commandDetails.setOverwrite(true)
		commandDetails.setMetadata(false)
		DimensionsResult result = baseline.download(commandDetails)
		//show("${msgOk} Descarga de componentes (DimensionsResult):    ${result.getMessage()}")
	}

	static void analisisProperties( String path )
	{
		show("${msgOk} Componentes descargados en: ${path}")
		def ip_prep = [ "10.0.157.148", "200.55.212.107", "10.210.2.28", "cdn.ripley.cl", "200.75.7.211", "10.0.156.170", "192.168.3.133" ]
        def ip_prod = [ "192.168.9.92", "www.bancoripley.cl", "192.168.80.34", "www.corepro.cl", "www.eftgroup.cl", "www.servipag.com" ]
        def matcher = null
        String patternUrl = /^.*(http:\/{2}|https:\/{2})([A-Za-z0-9\.]+)\/(.*)$/

        new File(path).eachFileRecurse() { file->
            if( (file.isFile()) && (file.name =~ /^.*.properties$/) ){
                //println file.getAbsolutePath()
                new File(file.getAbsolutePath()).eachLine { line ->
                    matcher = line =~ patternUrl
                    matcher.each { mat->
                    	String filePath = file.getAbsolutePath()-path
                        if(ip_prep.contains(mat[2]))
                           show("${msgInf} IP-PREPRODUCCION: '${mat[2]}' encontrada en archivo: ${filePath}")
                        if(ip_prod.contains(mat[2]))
                           show("${msgInf} IP-PRODUCCION: '${mat[2]}' encontrada en archivo: ${filePath}")
                    }
                }
            }
        }

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
        if(projects.size>0)
        	return (Project) projects.get(0);
        else
        	failed("${msgEr} EL PROYECTO ${productID}:${projectID} NO EXISTE");
    }

    static Project getProjectFromBaseline(List<Baseline> lista_baselines) 
    {
    	def lista_projects = lista_baselines.get(0).getParentProjects() 	
        Project dmproject = (Project) lista_projects.get(0).getObject()
        return dmproject
    }

    static void loadProperties() 
    {
        new File(executionPath, "data.properties").withInputStream {  propsDimensions.load(it) }
    }

    static DimensionsConnection crearConexion() 
    {
        // obteniendo la conexion con Dimensions          
        DimensionsConnectionDetails details = new DimensionsConnectionDetails()
        details.setUsername(propsDimensions['username'])
        details.setPassword(propsDimensions['password'])
        details.setDbName(propsDimensions['dbname'])
        details.setDbConn(propsDimensions['dbconn'])
        details.setServer(propsDimensions['ip'])
        return(DimensionsConnectionManager.getConnection(details))
    }
}
