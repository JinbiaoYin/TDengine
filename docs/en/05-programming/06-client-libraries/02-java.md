---
toc_max_heading_level: 4
sidebar_position: 2
sidebar_label: Java
title: TDengine Java Client Library
description: This document describes the TDengine Java client library.
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';


'taos-jdbcdriver' is TDengine's official Java language client library, which allows Java developers to develop applications that access the TDengine database. 'taos-jdbcdriver' implements the interface of the JDBC driver standard. 'taos-jdbcdriver' connect to a TDengine instance through the REST API.

## TDengine DataType vs. Java DataType

TDengine currently supports timestamp, number, character, Boolean type, and the corresponding type conversion with Java is as follows:

| TDengine DataType | JDBCType  |
| ----------------- | ---------------------------------- |
| TIMESTAMP         | java.sql.Timestamp                 |
| INT               | java.lang.Integer                  |
| BIGINT            | java.lang.Long                     |
| FLOAT             | java.lang.Float                    |
| DOUBLE            | java.lang.Double                   |
| SMALLINT          | java.lang.Short                    |
| TINYINT           | java.lang.Byte                     |
| BOOL              | java.lang.Boolean                  |
| BINARY            | byte array                         |
| NCHAR             | java.lang.String                   |
| JSON              | java.lang.String                   |

**Note**: Only TAG supports JSON types

## Installation steps

### Pre-installation preparation

Java 1.8 or above runtime environment and Maven 3.6 or above installed

### Install the client library

<Tabs defaultValue="maven">
<TabItem value="maven" label="Install via Maven">

- [sonatype](https://search.maven.org/artifact/com.taosdata.jdbc/taos-jdbcdriver)
- [mvnrepository](https://mvnrepository.com/artifact/com.taosdata.jdbc/taos-jdbcdriver)
- [maven.aliyun](https://maven.aliyun.com/mvn/search)

Add following dependency in the `pom.xml` file of your Maven project:

```xml
<dependency>
 <groupId>com.taosdata.jdbc</groupId>
 <artifactId>taos-jdbcdriver</artifactId>
 <version>3.2.9</version>
</dependency>
```

</TabItem>
<TabItem value="source" label="Build from source code">

You can build Java client library from source code after cloning the TDengine project:

```
git clone https://github.com/taosdata/taos-connector-jdbc.git
cd taos-connector-jdbc
mvn clean install -Dmaven.test.skip=true
```

After compilation, a jar package named taos-jdbcdriver-3.0.*-dist.jar is generated in the target directory, and the compiled jar file is automatically placed in the local Maven repository.

</TabItem>
</Tabs>

## Establish Connection using URL

TDengine's JDBC URL specification format is:
`jdbc:TAOS-RS://[host_name]:[port]/[database_name]?batchfetch={true|false}&useSSL={true|false}&token={token}&httpPoolSize={httpPoolSize}&httpKeepAlive={true|false}]&httpConnectTimeout={httpTimeout}&httpSocketTimeout={socketTimeout}`

```java
Class.forName("com.taosdata.jdbc.rs.RestfulDriver");
String jdbcUrl = System.getenv("TDENGINE_JDBC_URL");
Connection conn = DriverManager.getConnection(jdbcUrl);
```

:::note IMPORTANT

-  REST API is stateless. When using the JDBC REST connection, you need to specify the database name of the table and super table in SQL. For example.

  ```sql
  INSERT INTO test.t1 USING test.weather (ts, temperature) TAGS('California.SanFrancisco') VALUES(now, 24.6);
  ```

- If dbname is specified in the URL, JDBC REST connections will use `/rest/sql/dbname` as the URL for REST requests by default, and there is no need to specify dbname in SQL. For example, if the URL is `jdbc:TAOS-RS://127.0.0.1:6041/test`, then the SQL can be executed: 
  ```sql
  insert into test using weather(ts, temperature) tags('California.SanFrancisco') values(now, 24.6);
  ```
:::

### Establish Connection using URL and Properties

In addition to getting the connection from the specified URL, you can use Properties to specify parameters when the connection is established.

```java
public Connection getRestConn() throws Exception{
  Class.forName("com.taosdata.jdbc.rs.RestfulDriver");
  String jdbcUrl = System.getenv("TDENGINE_JDBC_URL");
  Properties connProps = new Properties();
  connProps.setProperty(TSDBDriver.PROPERTY_KEY_BATCH_LOAD, "true");
  Connection conn = DriverManager.getConnection(jdbcUrl, connProps);
  return conn;
}
```
### Priority of configuration parameters

If the configuration parameters are duplicated in the URL, Properties, the `priority` of the parameters, from highest to lowest, are as follows:

1. JDBC URL parameters, as described above, can be specified in the parameters of the JDBC URL.
2. Properties connProps

## Usage Examples

### Create Database and Tables

```java
Statement stmt = conn.createStatement();
// create database
stmt.executeUpdate("create database if not exists db");
// create table
stmt.executeUpdate("create table if not exists db.tb (ts timestamp, temperature int, humidity float)");
```

### Insert Data

```java
// insert data
int affectedRows = stmt.executeUpdate("insert into tb values(now, 23, 10.3) (now + 1s, 20, 9.3)");
System.out.println("insert " + affectedRows + " rows.");
```

> `now` is an internal function. The default is the current time of the client's computer.
> `now + 1s` represents the current time of the client plus 1 second, followed by the number representing the unit of time: a (milliseconds), s (seconds), m (minutes), h (hours), d (days), w (weeks), n (months), y (years).

### Querying data

```java
// query data
ResultSet resultSet = stmt.executeQuery("select * from tb");

Timestamp ts = null;
int temperature = 0;
float humidity = 0;
while(resultSet.next()){

    ts = resultSet.getTimestamp(1);
    temperature = resultSet.getInt(2);
    humidity = resultSet.getFloat("humidity");

    System.out.printf("%s, %d, %s\n", ts, temperature, humidity);
}
```

The query is consistent with operating a relational database. When using subscripts to get the contents of the returned fields, you have to start from 1. However, we recommend using the field names to get the values of the fields in the result set.

### Handling Exceptions

After an error is reported, the error message and error code can be obtained through SQLException.

```java
try (Statement statement = connection.createStatement()) {
    // executeQuery
    ResultSet resultSet = statement.executeQuery(sql);
    // print result
    printResult(resultSet);
} catch (SQLException e) {
    System.out.println("ERROR Message: " + e.getMessage());
    System.out.println("ERROR Code: " + e.getErrorCode());
    e.printStackTrace();
}
```

There are three types of error codes that the JDBC client library can report:

- Error code of the JDBC driver itself (error code between 0x2301 and 0x2350)
- Error code of the native connection method (error code between 0x2351 and 0x2400)
- Error code of other TDengine function modules

For specific error codes, please refer to.

- [TDengine Java Client Library](https://github.com/taosdata/taos-connector-jdbc/blob/main/src/main/java/com/taosdata/jdbc/TSDBErrorNumbers.java)

### Closing resources

```java
resultSet.close();
stmt.close();
conn.close();
```

:::note IMPORTANT
 Be sure to close the connection, otherwise, there will be a connection leak.
:::

### Use with Connection Pool

#### HikariCP

Example usage is as follows.

```java
 public static void main(String[] args) throws SQLException {
    HikariConfig config = new HikariConfig();
    // jdbc properties
    config.setJdbcUrl("jdbc:TAOS://127.0.0.1:6030/log");
    config.setUsername("root");
    config.setPassword("taosdata");
    // connection pool configurations
    config.setMinimumIdle(10);           //minimum number of idle connection
    config.setMaximumPoolSize(10);      //maximum number of connection in the pool
    config.setConnectionTimeout(30000); //maximum wait milliseconds for get connection from pool
    config.setMaxLifetime(0);       // maximum life time for each connection
    config.setIdleTimeout(0);       // max idle time for recycle idle connection
    config.setConnectionTestQuery("select server_status()"); //validation query

    HikariDataSource ds = new HikariDataSource(config); //create datasource

    Connection  connection = ds.getConnection(); // get connection
    Statement statement = connection.createStatement(); // get statement

    //query or insert
    // ...

    connection.close(); // put back to connection pool
}
```

> getConnection(), you need to call the close() method after you finish using it. It doesn't close the connection. It just puts it back into the connection pool.
> For more questions about using HikariCP, please see the [official instructions](https://github.com/brettwooldridge/HikariCP).

#### Druid

Example usage is as follows.

```java
public static void main(String[] args) throws Exception {

    DruidDataSource dataSource = new DruidDataSource();
    // jdbc properties
    dataSource.setDriverClassName("com.taosdata.jdbc.TSDBDriver");
    dataSource.setUrl(url);
    dataSource.setUsername("root");
    dataSource.setPassword("taosdata");
    // pool configurations
    dataSource.setInitialSize(10);
    dataSource.setMinIdle(10);
    dataSource.setMaxActive(10);
    dataSource.setMaxWait(30000);
    dataSource.setValidationQuery("select server_status()");

    Connection  connection = dataSource.getConnection(); // get connection
    Statement statement = connection.createStatement(); // get statement
    //query or insert
    // ...

    connection.close(); // put back to connection pool
}
```

For more questions about using druid, please see [Official Instructions](https://github.com/alibaba/druid).


### More sample programs

The source code of the sample application is under `TDengine/examples/JDBC`:

- JDBCDemo: JDBC sample source code.
- JDBCConnectorChecker: JDBC installation checker source and jar package.
- connectionPools: using taos-jdbcdriver in connection pools such as HikariCP, Druid, dbcp, c3p0, etc.
- SpringJdbcTemplate: using taos-jdbcdriver in Spring JdbcTemplate.
- mybatisplus-demo: using taos-jdbcdriver in Springboot + Mybatis.

Please refer to: [JDBC example](https://github.com/taosdata/TDengine/tree/develop/examples/JDBC)

## Recent update logs

| taos-jdbcdriver version |                 major changes                  |
| :---------------------: | :--------------------------------------------: |
|       3.0.3        |  fix timestamp resolution error for REST connection in jdk17+ version    |
|       3.0.1 - 3.0.2        |  fix the resultSet data is parsed incorrectly sometimes. 3.0.1 is compiled on JDK 11, you are advised to use 3.0.2 in the JDK 8 environment    |
|       3.0.0        |   Support for TDengine 3.0    |
|       2.0.42        |   fix wasNull interface return value in WebSocket connection  |
|       2.0.41        |   fix decode method of username and password in REST connection |
|     2.0.39 - 2.0.40     | Add REST connection/request timeout parameters |
|         2.0.38          |  JDBC REST connections add bulk pull function  |
|         2.0.37          |               Support json tags                |
|         2.0.36          |           Support schemaless writing           |

## Frequently Asked Questions

1. Why is there no performance improvement when using Statement's `addBatch()` and `executeBatch()` to perform `batch data writing/update`?

   **Cause**: In TDengine's JDBC implementation, SQL statements submitted by `addBatch()` method are executed sequentially in the order they are added, which does not reduce the number of interactions with the server and does not bring performance improvement.

   **Solution**: 1. splice multiple values in a single insert statement; 2. use multi-threaded concurrent insertion; 3. use parameter-bound writing

2. java.lang.UnsatisfiedLinkError: no taos in java.library.path

   **Cause**: The program did not find the dependent native library `taos`.

   **Solution**: On Windows you can copy `C:\TDengine\driver\taos.dll` to the `C:\Windows\System32` directory, on Linux the following soft link will be created `ln -s /usr/local/taos/driver/libtaos.so.x.x.x.x /usr/lib/libtaos.so` will work, on macOS the lib soft link will be `/usr/local/lib/libtaos.dylib`.

3. java.lang.UnsatisfiedLinkError: taos.dll Can't load AMD 64 bit on a IA 32-bit platform

   **Cause**: Currently, TDengine only supports 64-bit JDK.

   **Solution**: Reinstall the 64-bit JDK.

4. java.lang.NoSuchMethodError: setByteArray

  **Cause**: taos-jbdcdriver 3.* only supports TDengine 3.0 and later.

  **Solution**: Use taos-jdbcdriver 2.* with your TDengine 2.* deployment.

5. java.lang.NoSuchMethodError: java.nio.ByteBuffer.position(I)Ljava/nio/ByteBuffer; ... taos-jdbcdriver-3.0.1.jar

**Cause**：taos-jdbcdriver 3.0.1 is compiled on JDK 11.

## API Reference

[taos-jdbcdriver doc](https://docs.taosdata.com/api/taos-jdbcdriver)
