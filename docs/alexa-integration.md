# Alexa / Home Assistant integration

Coffee Sentinel does not talk to Alexa directly. It sends a plain HTTP
webhook; a home-automation hub turns that into a spoken announcement. This
keeps the firmware small and free of vendor SDKs, and lets you swap Alexa
for Google Home, a Slack message or a Telegram bot by changing one
automation.

```
ESP32 ──Wi-Fi──▶ HTTP POST /api/webhook/coffee-sentinel ──▶ Home Assistant ──▶ Alexa Media Player ──▶ "O café está pronto."
```

## 1. Firmware side

1. Copy `include/secrets.example.h` to `include/secrets.h`.
2. Fill in `kWifiSsid`, `kWifiPassword` and `kWebhookUrl`. Leave
   `kWebhookToken` empty for Home Assistant webhooks (the URL is the secret).
3. Flash: `pio run -e esp32dev -t upload`.

The payload sent when the coffee is ready:

```json
{"event":"coffee_ready","message":"Coffee is ready","device":"coffee-sentinel","version":"0.1.0"}
```

With `AppConfig::notifyOnReset = true` a second event `coffee_reset` is sent
when the light goes off again.

## 2. Home Assistant + Alexa Media Player (recommended)

Requirements: Home Assistant reachable from the ESP32's network and the
[Alexa Media Player](https://github.com/alandtse/alexa_media_player) custom
integration (HACS).

`configuration.yaml` is not needed; create the automation in the UI or paste
this YAML (Settings → Automations → ⋮ → Edit in YAML):

```yaml
alias: Coffee Sentinel → Alexa
description: Announce the coffee when the ESP32 webhook fires
mode: single
trigger:
  - platform: webhook
    webhook_id: coffee-sentinel          # must match the end of kWebhookUrl
    allowed_methods: [POST]
    local_only: true                     # the ESP32 is on the LAN
condition:
  - condition: template
    value_template: "{{ trigger.json.event == 'coffee_ready' }}"
action:
  - service: notify.alexa_media
    data:
      target:
        - media_player.kitchen_echo       # your Echo entity
      data:
        type: announce
      message: "O café está pronto."
  - service: persistent_notification.create
    data:
      title: Coffee Sentinel
      message: "{{ trigger.json.message }} ({{ now().strftime('%H:%M') }})"
```

Test it without the ESP32:

```bash
curl -X POST http://homeassistant.local:8123/api/webhook/coffee-sentinel \
     -H 'Content-Type: application/json' \
     -d '{"event":"coffee_ready","message":"Coffee is ready","device":"test"}'
```

## 3. Alternatives

| Route | When to use it | Notes |
| --- | --- | --- |
| **Home Assistant → Alexa Media Player** | You already run Home Assistant | Most flexible: announcements, TTS, routines, history |
| **Virtual Smart Home (VSH) skill** | No hub, but you have an Echo | Create a virtual doorbell/contact sensor, trigger it from the webhook, let an Alexa Routine speak. Needs a small bridge (Node-RED or a few lines of Python) to call their MQTT API |
| **IFTTT Webhooks → Alexa** | Quick experiment | IFTTT's Alexa integration is limited; announcements usually need a workaround |
| **Own service** | Full control | A 20-line Flask/Express endpoint that forwards to anything (Slack, Telegram, e-mail). The firmware does not care who is listening |

## 4. Security notes

* Home Assistant webhooks are authenticated only by the URL. Keep
  `webhook_id` long and random, enable `local_only: true`, and never commit
  `include/secrets.h`.
* HTTPS endpoints work (`WiFiClientSecure` without certificate validation in
  v0.2). Certificate pinning is on the roadmap; on a LAN this is an
  acceptable trade-off.
* The ESP32 only ever makes outbound requests; it does not open a server.
