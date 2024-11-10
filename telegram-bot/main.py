from fastapi import FastAPI, BackgroundTasks
from pydantic import BaseModel
from aiogram import Bot, Dispatcher, types
from aiogram.fsm.storage.memory import MemoryStorage
from aiogram import F
import asyncio
import logging
import os

API_TOKEN = ''
CHAT_ID = ''  # Chat ID where notifications will be sent

# Initialize FastAPI app and Aiogram bot
app = FastAPI()
bot = Bot(token=API_TOKEN)
dp = Dispatcher(storage=MemoryStorage())

monitoring = True
logging.basicConfig(level=logging.INFO)

# Request model for notification
class NotificationRequest(BaseModel):
    server: str

async def async_notify(server_ip: str):
    try:
        await bot.send_message(chat_id=CHAT_ID, text=f"Server {server_ip} not available now.")
        logging.info(f"Message sent for server {server_ip}")
    except Exception as e:
        logging.error(f"Failed to send message: {e}")

@app.post("/notify")
async def notify(request: NotificationRequest, background_tasks: BackgroundTasks):
    server_ip = request.server

    # Background task to send the notification
    background_tasks.add_task(async_notify, server_ip)

    return {"status": "notification sent"}

# Start monitoring command
@dp.message(F.command("start"))
async def start_monitoring(message: types.Message):
    global monitoring
    monitoring = True
    await message.reply("Server monitoring started.")

# Stop monitoring command
@dp.message(F.command("stop"))
async def stop_monitoring(message: types.Message):
    global monitoring
    monitoring = False
    await message.reply("Server monitoring stopped.")

# Function to poll for new log entries
async def monitor_log():
    if not os.path.exists("unavailable_servers.log"):
        return
    with open("unavailable_servers.log", "r") as log_file:
        lines = log_file.readlines()

    while monitoring:
        with open("unavailable_servers.log", "r") as log_file:
            new_lines = log_file.readlines()
            if len(new_lines) > len(lines):
                for line in new_lines[len(lines):]:
                    await bot.send_message(CHAT_ID, line.strip())
                lines = new_lines
        await asyncio.sleep(30)

# Start FastAPI and bot in an asynchronous event loop
@app.on_event("startup")
async def startup():
    logging.info("Starting bot and monitoring.")
    # Run the bot's message polling in the background
    asyncio.create_task(dp.start_polling())

@app.on_event("shutdown")
async def shutdown():
    await bot.close()
    logging.info("Bot shut down.")

# Main function for monitoring log in the background
async def monitor():
    await monitor_log()

if __name__ == "__main__":
    # Run FastAPI app
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=5000)
