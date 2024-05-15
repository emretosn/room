from supabase import create_client
import pandas as pd
import streamlit as st
import plotly.express as px

import os
from dotenv import load_dotenv

load_dotenv()
api_url = os.getenv('API_URL')
api_key = os.getenv('API_KEY')
tablename = 'table'

try:
    supabase = create_client(api_url, api_key)
except Exception as e:
    st.error(f"Error creating Supabase client: {e}")
    st.stop()

try:
    supabaseList = supabase.table(tablename).select('*').execute().data
except Exception as e:
    st.error(f"Error fetching data from Supabase: {e}")
    st.stop()

df = pd.DataFrame()
for row in supabaseList:
    row["created_at"] = row["created_at"].split(".")[0]
    row["time"] = row["created_at"].split("T")[1]
    row["date"] = row["created_at"].split("T")[0]
    row["DateTime"] = row["created_at"]
    df = pd.concat([df, pd.DataFrame([row])], ignore_index=True)

st.set_page_config(page_title="Your Room",layout='wide', initial_sidebar_state='collapsed')

response = supabase.table(tablename).select('*').order('id', desc=True).limit(2).execute()
cur_temp = response.data[0]['temperature']
p_temp   = response.data[1]['temperature']
cur_h    = response.data[0]['humidity']
p_h      = response.data[1]['humidity']
cur_hic  = response.data[0]['hic']
p_hic    = response.data[1]['hic']
cur_aq   = response.data[0]['airquality']
p_aq     = response.data[1]['airquality']
text = (f"Here, you have some information about your room, on the cloud! Monitor your room's "
        f"temperature, humidity, and air quality and use this information to turn on your AC or humidifier remotely.\n"
        f"The room is currently sitting at **{cur_temp}ºC** with **{cur_h}** humidity levels so it feels like **{cur_hic}ºC** in the room. "
        f"Also, the air quality index for the room is currently reading **{cur_aq}**.")


st.markdown('# Your Room')
st.markdown(text)


col1, col2 = st.columns(2)

col1.metric(label='Temperature', value=str(cur_temp)+'ºC', delta="{:.2f}".format(cur_temp-p_temp))
col2.metric(label='Humidity', value=cur_h, delta="{:.2f}".format(cur_h-p_h))
col1.metric(label='HIC', value=str(cur_temp)+'ºC', delta="{:.2f}".format(cur_hic-p_hic))
col2.metric(label='Air Quality', value=cur_aq, delta="{:.2f}".format(cur_aq-p_aq))

fig = px.line(df, x="DateTime", y="temperature", title='Temperature',markers=True)
col1.plotly_chart(fig,use_container_width=True)

fig = px.line(df, x="DateTime", y="humidity", title='Humidity',markers=True)
col2.plotly_chart(fig,use_container_width=True)

fig = px.line(df, x="DateTime", y="hic", title='Feels Like',markers=True)
col1.plotly_chart(fig,use_container_width=True)

fig = px.line(df, x="DateTime", y="airquality", title='Air Quality',markers=True)
col2.plotly_chart(fig,use_container_width=True)
