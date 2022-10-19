# Using a DB with concord

Sometimes, when you are developing a bot command, you may wish to save some data that will not be erased once the bot restart (aka persistent data). This is where a database comes in - it can save data, and don't worry, it won't be erased if you restart your bot.

In this guide, you'll be learning how to use sqlite3, a local database. It's incredibly fast, easy to use, and simple to debug.

## Opening the DB file

The first step in this guide will be opening the DB file, as it's required for the other sqlite3 functions like reading and writing data.

```c
#include <sqlite3.h> // Including the sqlite3 header so we can use its functions.

...

/* Here we are opening the DB file so other functions (read and save records) can be executed.
And also checking the status code of this function, to see if it failed or not to be executed. */

sqlite3 *db;
int rc = sqlite3_open("db.sqlite", &db);

if (rc != SQLITE_OK) {  // Checking if something failed while opening the DB file.
    /* As it saw something went wrong, it is going to log a fatal debug message with the error message of what went wrong. */
    log_fatal("[SQLITE] Error when opening the DB file. [%s]", sqlite3_errmsg(db));

    /* Since it failed, the resources must be deallocated. We are using the sqlite3_close for that. 
    If something went wrong while trying to close it, the code inside this if will be executed. (NOTE: Yes, even failing to open, you MUST use `sqlite3_close` as said in the sqlite3 docs!) */
      
    if (sqlite3_close(db) != SQLITE_OK) {
        /* Logging a fatal saying that it failed to close the DB. (NOTE: The sqlite3_errmsg function shows the error message of what happened) */
        log_fatal("[SQLITE] Failed to close sqlite DB. [%s]", sqlite3_errmsg(db));

        /* This is not a high-detailed guide, so we are not going to explain how to deal with this case with a lot of details. 
        But you will need to finalize the ongoing statement and execute the sqlite3_close again. */
        abort();
    }
    return;
}
```

> Whether or not an error occurs when it is opened, resources associated with the database connection handle should be released by passing it to sqlite3_close() when it is no longer required.

## Creating a sqlite3 table

Firstly, before trying to save data into the DB file, we need to create a SQL table (the schema for that DB). A DB schema can be summed up as "a description of all of the other tables, indexes, triggers, and views that are contained within the database".

```c
char *msgErr = NULL;
char *query = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS concord_guides(user_id INT, guild_id INT);");
rc = sqlite3_exec(db, query, NULL, NULL, &msgErr);

sqlite3_free(query);

if (rc != SQLITE_OK) {
    log_fatal("[SQLITE] Something went wrong while creating concord_guides table. [%s]", msgErr);
    if (sqlite3_close(db) != SQLITE_OK) {
        log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
        abort();
    }
    return;
}

sqlite3_close(db);
```

Here, we are defining the command that **will be executed** with sqlite3_mprintf, the command going to be executed is "`CREATE TABLE IF NOT EXISTS concord_guides(user_id INT, guild_id INT)`", where **if the concord_guides table doesn't exist**, it is going to be created.

Also, we are saving **INT values**, but if you wanted to **save a text value (like a string)**, you can simply replace `INT` with `TEXT` (WARNING: **This requires modifications to how you retrieve data**).

Then `sqlite3_exec` is going to perform the command defined with `sqlite3_mprintf`, and, **if some error happens**, it is going to write on the `char *msgErr`. After `sqlite3_exec`, we are going to free the query (generated by `sqlite3_mprintf`) to ensure resources are cleaned up and **no memory is leaked**.

After freeing the query, we **check if the status code of the `sqlite3_exec` is not equal to `SQLITE_OK`**, in case it's not, it's going to log a fatal saying that it failed that something failed while creating the table, and also includes the error message and closing the DB.

Done! If all goes well, then you've created a table called concord_guides.

## Saving records into the created table

Now that we created a table, we need to save records into it, it's the purpose of sqlite3.

For this, it will be pretty similar to the way that we created the table. Follow the example:

```c
query = sqlite3_mprintf("INSERT INTO concord_guides(user_id, guild_id) values(%"PRIu64", %"PRIu64");", msg->author->id, msg->guild_id);
rc = sqlite3_exec(db, query, NULL, NULL, &msgErr);

sqlite3_free(query);

if (rc != SQLITE_OK) {
  log_fatal("[SQLITE] Something went wrong while inserting values into concord_guides table. [%s]", msgErr);
  if (sqlite3_close(db) != SQLITE_OK) {
        log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
        abort();
    }
}

sqlite3_close(db);
```

Pretty similar, isn't it? The difference is, the command now is "`INSERT INTO concord_guides(user_id, guild_id) values(%"PRIu64", %"PRIu64");`".

The `concord_guides()` parameters **MUST match the schema for the table** that we wish to insert corresponding `values()` into.

## Reading the saved records

Finally, after saving the records, we will be reading them! This is a little more complicated than creating the table and saving records. See the following example:

```c
sqlite3_stmt *stmt = NULL;

query = sqlite3_mprintf("SELECT user_id FROM concord_guides WHERE guild_id = %"PRIu64";", msg->guild_id);
rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

if ((rc = sqlite3_step(stmt)) != SQLITE_ERROR) {
    log_trace("Sucessfully read the record: %lld.", sqlite3_column_int64(stmt, 0));
}

if (sqlite3_finalize(stmt) != SQLITE_OK) {
    log_fatal("[SQLITE] Error while executing function sqlite3_finalize.");
    abort();
}

sqlite3_close(db);
```

Here we replace `sqlite3_exec` with `sqlite3_prepare_v2`, as it gives us better control over how the SQL query is executed.

Now, the command is "`SELECT user_id FROM user_voice WHERE guild_id = %"PRIu64";`", where it wants to get the value of user_id, using as a search parameter the guild_id also saved before.

After using `sqlite3_mprintf` to generate the query, and preparing the statement with `sqlite3_prepare_v2`, we use `sqlite3_step` for **evaluating the statement**. If it returns SQLITE_ROW, then it is going to successfully print the records with `sqlite3_column_int64` (NOTE: **You may wish to use other `sqlite3_column_*` to get other types of records, like TEXT**).

## Deleting records

After creating a table, saving records, and reading them, you may finally wish to delete the records.

Deleting records (AKA data) is as easy (and pretty similar) to creating a table, see the following example:

```c
query = sqlite3_mprintf("DELETE FROM concord_guides WHERE guild_id = %"PRIu64";", msg->guild_id);
rc = sqlite3_exec(db, query, NULL, NULL, &msgErr);

if (rc != SQLITE_OK) {
    log_fatal("[SYSTEM] Something went wrong while deleting concord_guides table from guild_id. [%s]", msgErr);
    if (sqlite3_close(db) != SQLITE_OK) {
        log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
        abort();
    }
}

sqlite3_close(db);
```

In this command, we are **deleting the record that matches the search parameter of `guild_id = %"PRIu64"`**, which means that it will **delete the concord_guides record of the user_id** that has the guild_id the same as the guild_id of the guild that the command is being executed.

After it, apply the same process as saving records. If you wish for a better explanation, go back to this [section](#saving-records-into-the-created-table).

## Example bot with the code of all processes

The following bot has the single purpose of **summing this entire guide** in a simple program.

Below, you'll find **4 commands**: `.createtable`, `.insertdata`, `.retrievedata`, `.deletedata`.

The `.createtable` should be the first one to be executed, it **creates a table** called `concord_guides` so records can be saved in it.

`.insertdata` **inserts a record into the table created with `.createtable`**. It inserts the ID of the guild and the ID of the message's author.

The command `.retrievedata` **retrieves the user_id saved with `.insertdata`**, using the ID of the guild (guild_id) as a **parameter to search it** and then sends a message in the same channel with the user_id saved.

And finally, the `.deletedata`, which **deletes the record inserted with `.insertdata`** that the guild_id is the same as the ID of the guild that the command is being executed (NOTE: This **doesn't delete the table****, only the record).

```c
#include <string.h>
#include <stdlib.h>

#include <concord/discord.h>
#include <concord/log.h>

#include <sqlite3.h>

void on_ready(struct discord *client, const struct discord_ready *bot) {
    log_info("Logged in as %s!", bot->user->username);
}

void on_createtable(struct discord *client, const struct discord_message *msg) {
    sqlite3 *db;
    int rc = sqlite3_open("db.sqlite", &db);

    if (rc != SQLITE_OK) {
        log_fatal("[SQLITE] Error when opening the db file. [%s]", sqlite3_errmsg(db));
      
        if (sqlite3_close(db) != SQLITE_OK) {
            log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
            abort();
        }
        return;
    }

    char *msgErr = NULL;

    char *query = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS concord_guides(user_id INT, guild_id INT);");
    rc = sqlite3_exec(db, query, NULL, NULL, &msgErr);

    sqlite3_free(query);

    if (rc != SQLITE_OK) {
        log_fatal("[SQLITE] Something went wrong while creating concord_guides table. [%s]", msgErr);
        if (sqlite3_close(db) != SQLITE_OK) {
            log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
            abort();
        }
        return;
    }

    sqlite3_close(db);

    struct discord_create_message params = { .content = "Created the table." };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void on_insertdata(struct discord *client, const struct discord_message *msg) {
    sqlite3 *db;
    int rc = sqlite3_open("db.sqlite", &db);

    if (rc != SQLITE_OK) {
        log_fatal("[SQLITE] Error when opening the db file. [%s]", sqlite3_errmsg(db));
      
        if (sqlite3_close(db) != SQLITE_OK) {
            log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
            abort();
        }
        return;
    }

    char *msgErr = NULL;

    char * query = sqlite3_mprintf("INSERT INTO concord_guides(user_id, guild_id) values(%"PRIu64", %"PRIu64");", msg->author->id, msg->guild_id);
    rc = sqlite3_exec(db, query, NULL, NULL, &msgErr);

    sqlite3_free(query);

    if (rc != SQLITE_OK) {
        log_fatal("[SQLITE] Something went wrong while inserting values into concord_guides table. [%s]", msgErr);

        if (sqlite3_close(db) != SQLITE_OK) {
            log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
            abort();
        }
    }

    sqlite3_close(db);

    struct discord_create_message params = { .content = "Inserted record." };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void on_retrievedata(struct discord *client, const struct discord_message *msg) {
    sqlite3 *db;
    int rc = sqlite3_open("db.sqlite", &db);

    if (rc != SQLITE_OK) {
        log_fatal("[SQLITE] Error when opening the db file. [%s]", sqlite3_errmsg(db));
      
        if (sqlite3_close(db) != SQLITE_OK) {
            log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
            abort();
        }
        return;
    }

    sqlite3_stmt *stmt = NULL;

    char *query = sqlite3_mprintf("SELECT user_id FROM concord_guides WHERE guild_id = %"PRIu64";", msg->guild_id);
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

    if ((rc = sqlite3_step(stmt)) != SQLITE_ERROR) {
        char message[64];
        snprintf(message, sizeof(message), "Sucessfully read the record: %lld.\n", sqlite3_column_int64(stmt, 0));

        struct discord_create_message params = { .content = message };
        discord_create_message(client, msg->channel_id, &params, NULL);
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK) {
        log_fatal("[SQLITE] Error while executing function sqlite3_finalize.");
        abort();
    }

    sqlite3_close(db);
}


void on_deletedata(struct discord *client, const struct discord_message *msg) {
    sqlite3 *db;
    int rc = sqlite3_open("db.sqlite", &db);

    if (rc != SQLITE_OK) {
        log_fatal("[SQLITE] Error when opening the db file. [%s]", sqlite3_errmsg(db));
      
        if (sqlite3_close(db) != SQLITE_OK) {
            log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
            abort();
        }
        return;
    }

    char *msgErr = NULL;

    char *query = sqlite3_mprintf("DELETE FROM concord_guides WHERE guild_id = %"PRIu64";", msg->guild_id);
    rc = sqlite3_exec(db, query, NULL, NULL, &msgErr);

    if (rc != SQLITE_OK) {
        log_fatal("[SYSTEM] Something went wrong while deleting concord_guides table from guild_id. [%s]", msgErr);
        if (sqlite3_close(db) != SQLITE_OK) {
            log_fatal("[SQLITE] Failed to close sqlite db. [%s]", sqlite3_errmsg(db));
            abort();
        }
    }

    sqlite3_close(db);

    struct discord_create_message params = { .content = "Deleted record." };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

int main(void) {
    struct discord *client = discord_config_init("config.json");

    discord_add_intents(client, DISCORD_GATEWAY_MESSAGE_CONTENT);

    discord_set_on_ready(client, &on_ready);
    discord_set_on_command(client, ".createtable", &on_createtable);
    discord_set_on_command(client, ".insertdata", &on_insertdata);
    discord_set_on_command(client, ".retrievedata", &on_retrievedata);
    discord_set_on_command(client, ".deletedata", &on_deletedata);

    discord_run(client);
}
```