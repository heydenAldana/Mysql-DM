# **MySql Database Manager - TBBD 2**
El siguiente proyecto consiste en la creación de un programa que sea capaz de gestionar la base de datos de Mysql, así como también gestionar y consultar las tablas del sistema. Este programa se realiza mediante Qt Creator con el lenguaje de programación C++ en un entorno Linux (base Debian) y mediante el uso del (hasta el momento) driver QODBC (compatibles tanto con MariaDB como con MySQL) el cual es oficial de Qt SQL. Como es un proyecto con fines educativos, **NO** debe ser tomado como software profesional

La interfaz gráfica del proyecto es capaz de:
* Gestionar las conexiones (crear y editar conexiones, eliminar una conexión específica y hacer un borrado completo de las conexiones)
* Cambiar entre conexiones con autenticación requerida al hacerlo.
* Guardar las conexiones en un formato JSON con encriptacion XOR + Base64 (cifrado débil)
* Mostrar los objetos soportados por MySQL (sin information_schema) y obtener el DDL al hacer clic derecho en algún elemento de estos, mostrandolo en la consola del DBMS.
* Ejecutar comandos SQL y mostrar los resultados en una vista de tabla QTableWidget (para tablas y vistas).
* Mostrar los mensajes de error o de éxito al realizar operaciones SQL.
* Agregar tablas y vistas en una base de datos de manera visual mediante ventanas emergentes y amigables al usuario.
* Actualiza en tiempo real los cambios realizados en la base de datos.

Esta sección se divide en 4 partes:
1. Limitaciones y restricciones encontradas.
2. Estructura de archivos del proyecto.
3. Como se obtienen la información de las tablas del sistema.
4. Como se usa el programa.
5. Cómo instalar la base de datos y compilar el GUI.

***
***

## **1. Limitaciones y restricciones encontradas**

Durante el desarrollo del proyecto, cuando se implementó la parte de mostrar los objetos soportados en MySQL, he encontrado que:
1. Hay objetos como ```Paquetes``` (MySQl y MariaDB no lo soportan), ```Secuencias/Generadores``` (MySQL/MariaDB no soporta secuencias como objeto independiente. El equivalente funcional es **AUTO_INCREMENT**, que se define a nivel de columna) y ```Tablespaces``` (Solo se puede obtener mediante **```information_schema```** y NO es permitido en el proyecto) que NO pueden ser implementados.
2. Aunque MySQL y MAriaDB comparten muchas de sus funciones básicas, se requiere usar un controlador específico para cada uno. Por ejemplo, en MariaDB se utiliza **[MariaDB Unicode]**, y en MySQL existen tanto **[MySQL ODBC 9.5 Unicode Driver]** como **[MySQL ODBC 9.5 ANSI Driver]** (aplicable en Qt)
3. Los **Índices** en MySQL se obtienen mediante un query especial, ya que no existe un comando ```SHOW INDEXES```. Dicho query, pasando de parámetro el nombre de la base de datos, se construye así:
       ```SELECT DISTINCT index_name, table_name
           FROM mysql.innodb_index_stats
           WHERE database_name = '%1'
           AND stat_name = 'size'
           AND index_name != 'PRIMARY'
       ```
5. En el caso del cifrado de las contraseñas, Qt puro no incluye AES nativo sin librerías externas, por lo que se usa XOR + Base64.
6. Los JOINs automáticos en creación de vistas necesitan que sus FKs sean declaradas, de lo contrario se realiza un ```CROSS JOIN``` por defecto.

***
***

## **2. Estructura de archivos del proyecto.**

El proyecto, que fue desarrollado en C++ para la lógica y Qt para la interfaz gráfica, cuenta con una estructura de archivos que incluyen los .cpp y sus headers (.h), archivos de interfaz (.ui), un CMakeLists.txt generado automáticamente para Qt5 y Qt6 al momento de compilar y un archivo docker-compose.yml para la base de datos. A continuación se detalla cada uno de estos:

---

### **2.1. Interfaz principal - dbms_main**

En esta parte, se muestra la interfaz principal de donde arranca el programa, donde se despliega el gestor de conexiones, visualización de los objetos de la base de datos, una consola con debugger para ejecutar SQL y una vista de tablas para visualizar los comandos de SELECT y dmás qud devuelven resultados.

* **dbma_main.ui**: Contiene toda la interfaz en un formato pseudo HTML para visualizarlo en Qt en el modo edición.
* **dbms_main.h**: Contiene todas las cabeceras del main, y sus funciones mas relevantes son:
  * `void updateConnTree()` -> Actualiza la lista de conexiones en el gestor de conexiones.
  * `void showQueryResults(QSqlQuery& query)` -> Muestra en la QTableWidget los resultados obtenidos en una búsqueda.
  * `void changeToolsState(bool setActive)` -> Habilita o deshabilitas las herramientas del DBMS si hay o no hay conexiones.
  * `void refreshDbInfo(dbHandler* handler)` -> Actualiza todos los objetos de una base de datos al hacer cambios en ella.
  * `void showStatusMessage(const QString& msg, bool isError)` -> Responsable de mostrar los mensajes de debug y errores.
  * `QString generateDDL(QTreeWidgetItem* item)` -> Ayuda a generar el DDL mediante comandos del sistema ("```SHOW CREATE [OBJECT]```", para los indices se necesita el query "```SHOW INDEX FROM `DB_NAME`.`TABLE_NAME` WHERE Key_name = 'INDEX_NAME'```"),
  * `void loadSavedConnections()` -> Carga las conexiones del archivo guardado. Lo hace al arrancar el programa como cada vez que se actualizan las conexiones.  
* **dbms_main.cpp**: Se implementa la lógica de la interfaz, los disparadores de eventos para la interacciób con la base de datos y las conexiones, manejo de guardado de las conexiones, manejo de las ventanas emergentes para crer las tablas y vistas, y el despliegue de resultados y errores.

### **2.2. Manejador de base de datos - dbHandler**

He creado una clase base que representa una sesión de conexión individual hacia el SGBD, y cada instancia encapsula los datos y el ciclo de vida de una conexión activa.
* **dbhandler.h**: Contiene la cabecera de la clase y sus funciones más relevantes son:
  * `bool startSession(server, dbName, username, password, port)` -> Construye la cadena de conexión ODBC y abre la sesión con el SGBD. Utiliza `DRIVER={MySQL ODBC 9.5 Unicode Driver}` y asigna un UUID único a cada conexión para permitir múltiples sesiones simultáneas.
  * `void disconnectSession()` -> Cierra la sesión activa y elimina el registro de la conexión del pool de Qt (`QSqlDatabase`).
  * Getters para `serverName`, `dbName`, `dbUsername`, `dbPassword`, `connId`, `dbErrorMsg`.
* **dbhandler.cpp**: Implementa la lógica de conexión mediante QODBC. El UUID generado con `QUuid::createUuid()` garantiza que cada instancia tenga un identificador de conexión único dentro del sistema de Qt del proyecto, evitando colisiones entre múltiples sesiones abiertas.

---

### **2.3. Almacenamiento persistente de conexiones - connFile**

He creado esta clase la cual es responsable de serializar y deserializar las conexiones activas hacia un archivo JSON en disco.

* **connFile.h**: Contiene toda la lógica del almacenamiento de las conexiones. Sus funciones más relevantes son:
  * `static QString filePath()` -> Retorna la ruta del archivo `connections.json`, ubicado en el directorio de datos de la aplicación según el sistema operativo (`QStandardPaths::AppDataLocation`).
  * `static QString encrypt(const QString& plain)` -> Ofusca una cadena de texto mediante cifrado XOR con clave fija `0x5A`, seguido de codificación Base64. Se utiliza para almacenar contraseñas de forma ilegible por razones de seguridad.
  * `static QString decrypt(const QString& encoded)` -> Proceso inverso al cifrado: decodifica Base64 y aplica XOR para recuperar el texto original.
  * `static bool saveConnections(const QList<dbHandler*>& list)` -> Serializa la lista completa de conexiones activas a un arreglo JSON, cifrando la contraseña de cada una antes de escribirla en disco.
  * `static QList<ConnData> loadConnections()` -> Lee el archivo JSON, descifra las contraseñas y retorna una lista de structs `ConnData` con los datos de cada conexión guardada.

---

### **2.4. Diálogo de conexión - dbms_connHandler**

Esta interfaz es una ventana emergente la cual es responsable de gestionar el ingreso de los datos para crear, editar o autenticar una conexión al SGBD.

* **dbms_connhandler.ui**: Define la interfaz del diálogo con campos para Servidor, Base de Datos, Usuario, Contraseña y Puerto, junto a los botones de Conectar y Cancelar.
* **dbms_connhandler.h**: Contiene la cabecera del diálogo. Sus elementos más relevantes son:
  * `enum Mode { ModeNewConnection, ModeEditConnection, ModeValidate }` -> Define el comportamiento visual del diálogo según el contexto desde el que fue invocado. Es decir, dependiendo si se crea una nueva conexión, se valida o se edita la misma, hay campos que pueden estar habilitados para escribir o no lo estarán, con el fin de mejorar la experiencia del usuario.
  * `void prefillData(server, db, user, password)` -> Precarga los campos del formulario con los datos de una conexión existente. En `ModeValidate`, el campo de contraseña se deja vacío intencionalmente para que el usuario la reingrese.
  * `dbHandler* getHandler()` -> Retorna el puntero a la sesión creada exitosamente tras la autenticación.
* **dbms_connhandler.cpp**: Implementa la lógica del diálogo. Sus comportamientos clave son:
  * `applyMode()` -> Aplica restricciones visuales según el modo activo. En `ModeValidate`, los campos de Servidor, Base de Datos y Usuario se bloquean como solo lectura con un estilo visual diferenciado, permitiendo únicamente editar la contraseña.
  * `on_btnConnect_clicked()` -> Instancia un `dbHandler`, invoca `startSession()` con los datos del formulario (incluyendo el puerto), y acepta el diálogo si la conexión es exitosa o muestra el error retornado por el SGBD en caso contrario.

---

### **2.5. Creación visual de tablas - dbms_create_table**

Es una ventana emergente que permite al usuario definir y crear una nueva tabla en la base de datos activa mediante una interfaz gráfica, sin necesidad de escribir SQL manualmente (aunque limitada).

* **dbms_create_table.ui**: Define la interfaz con un `QLineEdit` para el nombre de la tabla (`leTableName`) y un `QTableWidget` (`twEditColumns`) para el grid de definición de columnas, junto a botones para agregar/eliminar filas y los botones de Crear y Cancelar.
* **dbms_create_table.h**: Contiene la cabecera del diálogo. Sus funciones más relevantes son:
  * `void setupColumnGrid()` -> Configura los encabezados y el comportamiento de redimensionado del `QTableWidget`.
  * `void addColumnRow()` -> Agrega una nueva fila al grid con sus widgets embebidos: `QComboBox` para el tipo de dato (`INT`, `VARCHAR`, `CHAR`, `TEXT`, `DECIMAL`, `FLOAT`, `DOUBLE`, `BOOLEAN`, `DATE`, `DATETIME`, `TIMESTAMP`, `TIME`, `BIGINT`, `SMALLINT`, `TINYINT`, `BLOB`) y `QCheckBox` centrados para PK, Not Null y Auto Increment.
  * `QString buildCreateTableSQL()` -> Construye el DDL `CREATE TABLE` leyendo cada fila del grid, aplicando longitud cuando aplica al tipo seleccionado, y agrupando las columnas marcadas como PK en una cláusula `PRIMARY KEY` al final.
* **dbms_create_table.cpp**: Implementa la lógica del diálogo. La sentencia generada se ejecuta directamente contra la conexión activa mediante `QSqlQuery`. El motor de almacenamiento usado por defecto es `InnoDB`.

---

### **2.6. Creación visual de vistas - dbms_create_view**

Es una ventana emergente que permite construir de manera visual una sentencia `CREATE VIEW` seleccionando tablas, columnas y condiciones de filtro, con previsualización SQL en tiempo real, de forma limitada.

* **dbms_create_view.ui**: Define la interfaz con un `QListWidget` para selección de tablas origen (`lwTables`), un `QListWidget` para selección de columnas con checkboxes (`lwColumns`), un `QTableWidget` para el constructor de condiciones WHERE (`twCriteria`), un `QPlainTextEdit` de solo lectura para la previsualización SQL (`ptePreviewSql`), un `QLineEdit` para el nombre de la vista (`leViewName`) y botones de agregar/eliminar criterio, Crear Vista y Cancelar.
* **dbms_create_view.h**: Contiene la cabecera del diálogo. Sus funciones más relevantes son:
  * `void loadTables()` -> Carga las tablas disponibles en la base de datos activa usando `SHOW FULL TABLES ... WHERE Table_type = 'BASE TABLE'`.
  * `void loadColumnsForSelectedTables()` -> Para cada tabla marcada, ejecuta `SHOW COLUMNS FROM` y puebla `lwColumns` con las columnas disponibles como items con checkbox. Almacena `tabla.columna` en `Qt::UserRole` para uso en el SQL generado.
  * `QString buildJoinClause(const QStringList& tables)` -> Detecta relaciones entre tablas parseando el resultado de `SHOW CREATE TABLE` con una expresión regular sobre las cláusulas `FOREIGN KEY ... REFERENCES`. Si no encuentra FK declarada, recurre a `CROSS JOIN` como fallback.
  * `QString buildWhereClause()` -> Recorre las filas del `twCriteria` y construye la cláusula WHERE concatenando condiciones con `AND`. Detecta si el valor ingresado es numérico para omitir las comillas correspondientemente.
  * `QString buildCreateViewSQL()` -> Ensambla el DDL completo `CREATE VIEW` combinando las columnas seleccionadas, la cláusula FROM, los JOINs detectados y el WHERE construido.
  * `void updateSqlPreview()` -> Conectada a todos los eventos de cambio de la interfaz para actualizar `ptePreviewSql` en tiempo real.
  * `void updateColumnCombo(int row, const QString& tableName)` -> Actualiza el `QComboBox` de campos en una fila del criterio según la tabla seleccionada en esa misma fila, usando un mapa en caché (`tableColumnsMap`) para evitar queries repetidas.
* **dbms_create_view.cpp**: Implementa toda la lógica descrita. La sentencia final se ejecuta directamente contra la conexión activa. **Limitación documentada:** la detección automática de JOINs requiere que las tablas tengan FKs declaradas explícitamente; tablas sin FKs producirán un `CROSS JOIN`.

***
***

## **3. Como se obtienen la información de las tablas del sistema.**

## 3. Uso de Tablas del Sistema (System Tables)

El proyecto trabaja exclusivamente con las tablas del sistema y comandos nativos del SGBD para obtener la metadata, y asi se cumple con la restricción de no utilizar `information_schema` en ningún punto del código.

A continuación se detalla cada objeto soportado, el comando o system table utilizado, y porqué se usa dentro del proyecto.

---

### **3.1. Tablas**
* **Listado:** Se utiliza el comando `SHOW FULL TABLES IN {DB} WHERE Table_type = 'BASE TABLE'`, que retorna únicamente las tablas base, excluyendo las vistas.
* **DDL:** Se utiliza `SHOW CREATE TABLE {DB}.{TABLE}`, cuya columna 1 contiene el `CREATE TABLE` completo tal como fue definido originalmente, incluyendo constraints y motor de almacenamiento.
* **Mostrar columnas de tablas**: Se realiza mediante el comando `"SHOW COLUMNS FROM ``DB_NAME``.``DB_NTABLE`"

---

### **3.2. Vistas**
* **Listado:** Se reutiliza `SHOW FULL TABLES IN {DB} WHERE Table_type = 'VIEW'`, filtrando exclusivamente los objetos de tipo vista.
* **DDL:** Se utiliza `SHOW CREATE VIEW {TABLE}`, cuya columna 1 contiene el `CREATE VIEW` completo con la sentencia SELECT que la define.

---

### **3.3. Procedimientos Almacenados**
* **Listado:** Se utiliza `SHOW PROCEDURE STATUS WHERE Db = '{DB}'`, que retorna todos los procedimientos del esquema indicado. El nombre del procedimiento se obtiene de la columna 1 (`Name`).
* **DDL:** Se utiliza `SHOW CREATE PROCEDURE {NAME}`, cuya columna 2 contiene el `CREATE PROCEDURE` completo incluyendo el cuerpo del procedimiento.

---

### **3.4. Funciones**
* **Listado:** Se utiliza `SHOW FUNCTION STATUS WHERE Db = '{DB}'`, análogo al de procedimientos. El nombre se obtiene igualmente de la columna 1 (`Name`).
* **DDL:** Se utiliza `SHOW CREATE FUNCTION {NAME}`, cuya columna 2 contiene el `CREATE FUNCTION` completo.

---

### **3.5. Triggers**
* **Listado:** Se utiliza `SHOW TRIGGERS FROM {DB}`, que retorna todos los disparadores del esquema. El nombre del trigger se obtiene de la columna 0 (`Trigger`).
* **DDL:** Se utiliza `SHOW CREATE TRIGGER {NAME}`, cuya columna 2 contiene el `CREATE TRIGGER` completo incluyendo el evento y el cuerpo.

---

### **3.6. Índices**
* **Listado:** Se consulta la system table `mysql.innodb_index_stats` con la siguiente sentencia:
```sql
  SELECT DISTINCT index_name, table_name
  FROM mysql.innodb_index_stats
  WHERE database_name = '{DB}'
  AND stat_name = 'size'
  AND index_name != 'PRIMARY'
```
  Se excluye `PRIMARY` porque ese índice ya está representado en el DDL de la tabla. El `table_name` se almacena en el item del árbol usando `Qt::UserRole` para poder reconstruir el DDL posteriormente.
* **DDL:** A diferencia del resto de objetos, MySQL/MariaDB no provee un comando `SHOW CREATE INDEX`. Por ello, el DDL se **reconstruye manualmente** a partir del resultado de:
```sql
  SHOW INDEX FROM {DB}.{TABLE} WHERE Key_name = '{INDEX_NAME}'
```
  De este resultado se extraen los campos `Non_unique` (para determinar si es `UNIQUE`), `Index_type` (para el `USING BTREE/HASH`) y `Column_name` agrupados por `Seq_in_index` para respetar el orden de las columnas compuestas. Con estos datos se construye un `CREATE INDEX` equivalente.

---

### **3.7. Usuarios**
* **Listado:** Se consulta directamente la system table `mysql.user` con:
```sql
  SELECT CONCAT(User, '@', Host) FROM mysql.user
```
  Esto retorna todos los usuarios registrados en el SGBD en formato `usuario@host`.
* **DDL:** Se utiliza `SHOW CREATE USER '{USER}'@'{HOST}'`, cuya columna 0 contiene el `CREATE USER` completo incluyendo el método de autenticación. Para separar correctamente el usuario del host se hace un split por el último `@` del texto del item, respetando nombres de usuario que pudieran contener `@`.

---

### **3.8. Columnas (uso interno en creación de vistas)**
* No se expone como objeto independiente en el árbol, pero se consulta internamente en `dbms_create_view` para poblar los selectores de columnas:
```sql
  SHOW COLUMNS FROM {DB}.{TABLE}
```
  De este resultado se extraen el nombre del campo (columna 0) y el tipo de dato (columna 1) para presentarlos al usuario al momento de construir visualmente una vista.

---

### **3.9. Objetos no aplicables al SGBD**

| Objeto | Justificación |
|---|---|
| Paquetes | MySQL/MariaDB no implementa paquetes. Son un objeto exclusivo de Oracle PL/SQL y PostgreSQL. |
| Secuencias/Generadores | MySQL/MariaDB no soporta secuencias como objeto independiente. El equivalente funcional es `AUTO_INCREMENT`, definido a nivel de columna al crear la tabla. |
| Tablespaces | MySQL solo cuenta con `Tablespaces` en information_schema, y dado que `information_schema` está prohibido por las restricciones del proyecto, este objeto no es implementado. Todavia se puede consultar en el área de consola de SQL con "`SELECT * FROM information_schema.tablespaces;`". |

***
***

## **4. Como se usa el programa.**

Cuando se ejecuta el programa, por defecto, siempre aparece la ventana del dbms_main. Aquí es donde usted puede visualizar 4 secciones importantes:
* Gestor y visualizador de conexiones (añadir, editar, eliminar una y eliminar todas las conexiones) en la esquina izquierda superior.
* Visualizador de objetos de la base de datos a la que se conecta en la esquina izquierda inferior.
* Pseudo-consola SQL con debugger (mensajes de error o notificación) para ejecutar SQLs y visualizar/exportar los DDLs generados en la esquina derecha superior.
* Visualizador de tabla para mostrar resultados de un SELECT, con opciones para crear tablas y vistas cuya vista interactiva la hace fácil de usar, y se encuentra en la esquina derecha inferior.

***
***

## **Cómo instalar la base de datos y compilar el GUI.**

Esta sección cubre los pasos para levantar la base de datos MySQL mediante contenedor usando docker o podman y compilar la interfaz gráfica del proyecto.

---

### **5.1. Requisitos previos**

Antes de comenzar, por favor asegúrese de tener instalado lo siguiente:

| Requisito | Versión mínima | ¿Cómo verifico que versión tengo? |
|---|---|---|
| Docker o Podman | Cualquiera reciente | `docker --version` / `podman --version` |
| Docker Compose o Podman Compose | v2+ | `docker compose version` |
| Qt Creator + Qt SDK | Qt 5 o Qt 6 | Qt Installer |
| CMake | 3.16+ | `cmake --version` |
| Compilador C++ | GCC con C++17 | `g++ --version` |
| MySQL ODBC Connector | 9.5 | `odbcinst -q -d` |
| unixODBC | Cualquiera | `odbcinst --version` |

---

### **5.2. Levantar la base de datos con Docker o Podman**

El proyecto incluye un `docker-compose.yml` que levanta un contenedor de **MySQL 8.0** en el puerto **3307** del host, con almacenamiento persistente en `./mysql-data`. Estp signiifca que las bases de datos y tablas y demás que usted cree persistirán.

#### Opción A - Docker
```bash
# Desde la raíz del repositorio
docker compose up -d
```

#### Opción B - Podman
```bash
# Desde la raíz del repositorio
podman compose up -d
```

Una vez levantado, verifica que el contenedor esté activo:
```bash
# en docker
docker ps
# en podman
podman ps
```

Deberia poder ver `mysql-native` con estado `Up`.

**Datos de conexión del contenedor:**

| Campo | Valor |
|---|---|
| Servidor | `127.0.0.1` |
| Puerto | `3307` |
| Usuario | `root` |
| Contraseña | `mysql123` |
| Base de datos | (dejar vacío para ver todas) |

> **Nota:** El directorio `./mysql-data` se crea automáticamente en la raíz del repositorio la primera vez que se levanta el contenedor. Este directorio contiene los datos persistentes de MySQL y **no debe eliminarse** entre reinicios.

#### Detener el contenedor
```bash
# en docker
docker compose down
# en podman
podman compose down
```

---

### **5.3. Instalar el conector MySQL ODBC**

El proyecto requiere el driver **MySQL ODBC 9.5 Unicode Driver** para que Qt pueda comunicarse con MySQL.

**Paso 1 - Instalar dependencia unixODBC:**
```bash
# En distribuciones linux base Debian (si no es, buscar el equivalente de tu distribución):
sudo apt install unixodbc unixodbc-dev
```

**Paso 2 - Descargar el .deb oficial desde MySQL:**

Puedes obtener el .deb oficial aqui: https://downloads.mysql.com/archives/c-odbc/

**Paso 3 - Instalar el conector:**
```bash
sudo dpkg -i nombre_del_archivo.deb
```

Si falla por dependencias:
```bash
sudo apt --fix-broken install
sudo dpkg -i nombre_del_archivo.deb
```

**Paso 4 - Verificar que el driver quedó registrado:**
```bash
odbcinst -q -d
```

La salida debe incluir:
```
[MySQL ODBC 9.5 Unicode Driver]
[MySQL ODBC 9.5 ANSI Driver]
```

Si no aparece, el driver no está registrado y la aplicación no podrá conectarse.

---

### **5.4. Compilar la interfaz gráfica**

#### Opción A - Desde Qt Creator (recomendado)

1. Abre Qt Creator.
2. Ve a **File → Open File or Project** y selecciona el archivo `CMakeLists.txt` de la raíz del repositorio.
3. Qt Creator detectará automáticamente el kit de compilación disponible (Qt 5 o Qt 6).
4. Haz clic en el botón **Build** (ícono de martillo) o presiona `Ctrl+B`.
5. Una vez compilado, ejecuta con el botón **Run** o `Ctrl+R`.

#### Opción B - Desde terminal con CMake
```bash
# Desde la raíz del repositorio
mkdir build && cd build
cmake ..
make -j$(nproc)
./MySQL_DBMS
```

> **Nota:** Si tienes tanto Qt5 como Qt6 instalados, CMake dará prioridad a Qt6 automáticamente según el `CMakeLists.txt`. Para forzar el uso de Qt5 específicamente:
> ```bash
> cmake .. -DQT_VERSION_MAJOR=5
> ```

---

### **5.5. Primer uso**

1. Levanta el contenedor con `docker compose up -d` o `podman compose up -d`.
2. Ejecuta la aplicación.
3. En la ventana principal, haz clic en el botón **+** del gestor de conexiones.
4. Ingresa los datos del contenedor (ver tabla en sección 5.2).
5. Haz clic en **Conectar**.
6. La conexión quedará guardada automáticamente en `connections.json` para sesiones futuras.
