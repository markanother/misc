import org.apache.spark.sql.Row
import org.apache.spark.sql.SparkSession
import org.apache.spark.sql.types.{LongType, StringType, StructType}
// $example off:spark_hive$

object Test {

  // $example on:spark_hive$
  case class Record(key: Long, value: String)
  // $example off:spark_hive$

  def main(args: Array[String]) {
    // When working with Hive, one must instantiate `SparkSession` with Hive support, including
    // connectivity to a persistent Hive metastore, support for Hive serdes, and Hive user-defined
    // functions. Users who do not have an existing Hive deployment can still enable Hive support.
    // When not configured by the hive-site.xml, the context automatically creates `metastore_db`
    // in the current directory and creates a directory configured by `spark.sql.warehouse.dir`,
    // which defaults to the directory `spark-warehouse` in the current directory that the spark
    // application is started.

    // $example on:spark_hive$
    // warehouseLocation points to the default location for managed databases and tables
    //val warehouseLocation = new File("spark-warehouse").getAbsolutePath

    val spark = SparkSession
      .builder()
      .appName("Spark Hive Example")
      .config("spark.sql.warehouse.dir", "/user/zyh/spark_warehouse")
      .enableHiveSupport()
      .getOrCreate()

    import spark.implicits._
    import spark.sql

    sql("CREATE TABLE IF NOT EXISTS src (key LONG, value STRING) USING hive")
    //sql("LOAD DATA LOCAL INPATH 'examples/src/main/resources/kv1.txt' INTO TABLE src")
    sql("LOAD DATA INPATH '/user/zyh/kv.txt' INTO TABLE src")
    // Queries are expressed in HiveQL
    //sql("SELECT * FROM src").show()  

    // Aggregation queries are also supported.
    //sql("SELECT COUNT(*) FROM src").show()    

    // The results of SQL queries are themselves DataFrames and support all normal functions.
    //val sqlDF = sql("SELECT key, value FROM src WHERE key < 10 ORDER BY key")

    // The items in DataFrames are of type Row, which allows you to access each column by ordinal.
    //val stringsDS = sqlDF.map {
    //  case Row(key: Int, value: String) => s"Key: $key, Value: $value"
    //}
    //stringsDS.show()

    // You can also use DataFrames to create temporary views within a SparkSession.
    //val recordsDF = spark.createDataFrame((1 to 100).map(i => Record(i, s"val_$i")))
    //recordsDF.createOrReplaceTempView("records")
    val mySchema = new StructType().add("key", LongType).add("value", StringType)
    val recordsDF = spark
      .readStream
      .schema(mySchema)
      .json("/user/zyh/ss")
    recordsDF.createOrReplaceTempView("records")
    // Queries can then join DataFrame data with data stored in Hive.
    val query = sql("SELECT * FROM records r JOIN src s ON r.key = s.key")
      .writeStream
      .format("console")
      .outputMode("update")
      .start()

    query.awaitTermination()
  }
}
