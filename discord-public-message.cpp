#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libdiscord.h>

#include "discord-common.h"

namespace discord {
namespace message {

void
json_load(void *p_message, char *str, size_t len)
{
  data *message = (data*)p_message;

  struct json_token token_author = {NULL, 0};
  struct json_token token_mentions = {NULL, 0};
  struct json_token token_referenced_message = {NULL, 0};

  json_scanf(str, len,
     "[id]%s"
     "[channel_id]%s"
     "[guild_id]%s"
     "[author]%T"
     "[content]%s"
     "[timestamp]%s"
     "[edited_timestamp]%s"
     "[tts]%b"
     "[mention_everyone]%b"
     "[mentions]%T"
     "[nonce]%s"
     "[pinned]%b"
     "[webhook_id]%s"
     "[type]%d"
     "[flags]%d"
     "[referenced_message]%T",
      message->id,
      message->channel_id,
      message->guild_id,
      &token_author,
      message->content,
      message->timestamp,
      message->edited_timestamp,
      &message->tts,
      &message->mention_everyone,
      &token_mentions,
      message->nonce,
      &message->pinned,
      message->webhook_id,
      &message->type,
      &message->flags,
      &token_referenced_message);

  user::json_load(message->author, token_author.start, token_author.length);

  D_NOTOP_PUTS("Message object loaded with API response"); 
}

static data*
referenced_message_init()
{
  data *new_message = (data*)calloc(1, sizeof *new_message);
  if (NULL == new_message) return NULL;

  new_message->author = user::init();
  if (NULL == new_message->author) goto cleanup;

  return new_message;

cleanup:
  free(new_message);

  return NULL;
}

data*
init()
{
  data *new_message = (data*)calloc(1, sizeof *new_message);
  if (NULL == new_message) return NULL;

  new_message->author = user::init();
  if (NULL == new_message->author) goto cleanupA;
  
  new_message->referenced_message = referenced_message_init();
  if (NULL == new_message->referenced_message) goto cleanupB;

  return new_message;

cleanupB:
  free(new_message->author);
cleanupA:
  free(new_message);

  return NULL;
}

static void
referenced_message_cleanup(data *message)
{
  user::cleanup(message->author);

  free(message);
}

void
cleanup(data *message)
{
  user::cleanup(message->author);
  referenced_message_cleanup(message->referenced_message);

  free(message);
}

namespace create {

void
run(client *client, const char channel_id[], params *params)
{
  if (IS_EMPTY_STRING(channel_id)) {
    D_PUTS("Can't send message to Discord: missing 'channel_id'");
    return;
  }
  if (IS_EMPTY_STRING(params->content)) {
    D_PUTS("Can't send an empty message to Discord: missing 'content'");
    return;
  }
  if (strlen(params->content) >= MAX_MESSAGE_LEN) {
    D_PRINT("Content length exceeds 2000 characters threshold (%ld)", strlen(params->content));
    return;
  }

  char payload[MAX_PAYLOAD_LEN];
  int ret = snprintf(payload, MAX_PAYLOAD_LEN, "{\"content\":\"%s\"}", params->content);
  ASSERT_S(ret < MAX_PAYLOAD_LEN, "Out of bounds write attempt");

  user_agent::run( 
    &client->ua,
    NULL,
    NULL,
    payload,
    POST, CHANNEL MESSAGES, channel_id);
}

} // namespace create

} // namespace message
} // namespace discord