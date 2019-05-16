from __future__ import print_function
import pickle
import os.path
import argparse
from googleapiclient.discovery import build
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request
from po2csv import po2dict, dict2PO

# If modifying these scopes, delete the file token.pickle.
SCOPES = ['https://www.googleapis.com/auth/spreadsheets.readonly']

# The ID and range of a sample spreadsheet.
SAMPLE_SPREADSHEET_ID = '1iZa4wCIyTDlIRYoW7229YoZWKZ0lmIiOFsCJG3ZVw-s'

def google_get_creds():
    creds = None
    # The file token.pickle stores the user's access and refresh tokens, and is
    # created automatically when the authorization flow completes for the first
    # time.
    if os.path.exists('token.pickle'):
        with open('token.pickle', 'rb') as token:
            creds = pickle.load(token)
    # If there are no (valid) credentials available, let the user log in.
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(
                'credentials.json', SCOPES)
            creds = flow.run_local_server()
        # Save the credentials for the next run
        with open('token.pickle', 'wb') as token:
            pickle.dump(creds, token)
    
    return creds

def google_get_po_items(locale_code):
    SAMPLE_RANGE_NAME = locale_code + '!A:C'
    creds = google_get_creds()
    service = build('sheets', 'v4', credentials=creds)

    # Call the Sheets API
    sheet = service.spreadsheets()
    result = sheet.values().get(spreadsheetId=SAMPLE_SPREADSHEET_ID,
                                range=SAMPLE_RANGE_NAME).execute()
    values = result.get('values', [])

    po_items = {}
    if not values:
        print('No data found.')
    else:
        i = 2
        for row in values[1:]:
            # Print columns A and E, which correspond to indices 0 and 4.
            #print('%s, %s, %s' % (row[0], row[1], row[2]))
            #print(row)
            msgid = row[0][1:-1]
            msgstr = ''
            contrib = ''
            if (len(row)>1):
                msgstr = row[1][1:-1]
            if (len(row)>2):
                contrib = row[2]
            po_items[msgid] = [msgstr, contrib]
            i = i + 1
    return po_items

def google_remove_po_items(locale_code, po_items):
    creds = google_get_creds()
    service = build('sheets', 'v4', credentials=creds)

    # Call the Sheets API
    sheet = service.spreadsheets()

    remove_rows = []
    for msgid, val in po_items.items():
        remove_rows.append(locale_code + '!A' + str(val[0]))
        remove_rows.append(locale_code + '!B' + str(val[0]))
        remove_rows.append(locale_code + '!C' + str(val[0]))
    
    if (len(remove_rows) > 0):
        batch_clear_values_request_body = { 'ranges': remove_rows } 
        result = sheet.values().batchClear(spreadsheetId=SAMPLE_SPREADSHEET_ID, 
            body=batch_clear_values_request_body).execute()

def google_append_po_items(locale_code, po_items):
    creds = google_get_creds()
    service = build('sheets', 'v4', credentials=creds)

    # Call the Sheets API
    sheet = service.spreadsheets()
    value_input_option = 'RAW'
    insert_data_option = 'INSERT_ROWS'
    value_range_body = {}

    for msgid, val in po_items.items():
        result = sheet.values().append(spreadsheetId=SAMPLE_SPREADSHEET_ID, 
            range=range_, valueInputOption=value_input_option, 
            insertDataOption=insert_data_option, 
            body=value_range_body).execute()
    

parser = argparse.ArgumentParser(description='Google Sheets Utils')
parser.add_argument('--output',required=True,  dest='output_po_file', help='path of an output po_file that is created from Google Sheet')
parser.add_argument('locale_code', type=str, help='local code: e.g. es, zh_CN, de')
args = parser.parse_args()

if __name__ == "__main__":
    locale_code = args.locale_code
    output_po = args.output_po_file

    google_items = google_get_po_items(locale_code)
    dict2PO(google_items, output_po)
        
        
