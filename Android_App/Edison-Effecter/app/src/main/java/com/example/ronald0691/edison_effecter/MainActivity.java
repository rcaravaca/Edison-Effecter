package com.example.ronald0691.edison_effecter;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private BluetoothAdapter mBtAdapter;
    private ArrayAdapter<String> mPairedDevicesArrayAdapter;

    private static String GROVE_SERVICE = "0000ffe0-0000-1000-8000-00805f9b34fb";
    private static String CHARACTERISTIC_TX = "0000ffe1-0000-1000-8000-00805f9b34fb";
    private static String CHARACTERISTIC_RX = "0000ffe1-0000-1000-8000-00805f9b34fb";

    private static final int REQUEST_ENABLE_BT = 1;
    private static final long SCAN_PERIOD = 5000; //5 seconds
    private static final String DEVICE_NAME = "GE_"; //display name for Grove BLE

    private BluetoothAdapter mBluetoothAdapter;//our local adapter
    private BluetoothGatt mBluetoothGatt; //provides the GATT functionality for communication
    private BluetoothGattService mBluetoothGattService; //service on mBlueoothGatt
    private static List<BluetoothDevice> mDevices = new ArrayList<BluetoothDevice>();//discovered devices in range
    private BluetoothDevice mDevice; //external BLE device (Grove BLE module)

    private TextView mainText;
    private Timer mTimer;

    //**********************************************************************************************
    private TextView low_text;
    private TextView mid_text;
    private TextView high_text;
    private TextView vol_text;

    private SeekBar vol=null;
    private SeekBar low_euq=null;
    private SeekBar mid_euq=null;
    private SeekBar high_euq=null;

    private Switch Delay_switch;
    private Switch Reverb_switch;
    private Switch Overdrive_switch;

    int progressChanged_vol = 5;
    int progressChanged_low = 5;
    int progressChanged_mid = 5;
    int progressChanged_high = 5;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mTimer = new Timer();

        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "BLE not supported on this device", Toast.LENGTH_SHORT).show();
            finish();
        }

        //statusUpdate("BLE supported on this device");


        //get a reference to the Bluetooth Manager
        final BluetoothManager mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) {
            Toast.makeText(this, "BLE not supported on this device", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        //Open settings if Bluetooth isn't enabled
        if (!mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }

        if (mBluetoothAdapter == null) {
            Toast.makeText(this, "Bluetooth disabled", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        //try to find the Grove BLE V1 module
        Button button = (Button) findViewById(R.id.ble_connect);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                searchForDevices();
            }
        });



        //******************************************************************************************

        delay(); //Activacion de delay
        reverb(); //Activacion de reverb
        overdrive(); //Activacion de overdrive

        vol(); //Barra par control de volumen
        low(); //Barra par control de bajas frecuencias
        mid(); //Barra par control de medias frecuencias
        high(); //Barra par control de altas frecuencias





    }


    public void delay( ){

        Delay_switch = (Switch) findViewById(R.id.Switch_delay);
        Delay_switch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                                         boolean isChecked) {

                if(isChecked){
                    sendMessage("41");
                }else{
                    sendMessage("42");
                }

            }
        });

    }

    public void reverb( ){

        Reverb_switch = (Switch) findViewById(R.id.switch_reverb);
        Reverb_switch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                                         boolean isChecked) {

                if(isChecked){
                    sendMessage("51");
                }else{
                    sendMessage("52");
                }
            }
        });

    }

    public void overdrive( ){

        Overdrive_switch = (Switch) findViewById(R.id.switch_overdrive);
        Overdrive_switch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                                         boolean isChecked) {

                if(isChecked){
                    sendMessage("61");
                }else{
                    sendMessage("62");
                }
            }
        });


    }

    public void vol( ){

        vol = (SeekBar) findViewById(R.id.vol_Bar);
        vol.setProgress(4);
        vol.incrementProgressBy(1);
        vol.setMax(9);

        vol_text = (TextView)findViewById(R.id.vol_text);
        vol_text.setText("VOL: " + vol.getProgress());

        vol.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {


            public void onProgressChanged(SeekBar seekBar, int progress_vol, boolean fromUser){
                progressChanged_vol = progress_vol;
                vol_text.setText("VOL: " + progress_vol);


            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                vol_text.setText("VOL: " + progressChanged_vol);
                sendMessage("0"+Integer.toString(progressChanged_vol));
            }
        });

    }

    public void low( ){

        low_euq = (SeekBar) findViewById(R.id.low_bar);
        low_euq.setProgress(4);
        low_euq.incrementProgressBy(1);
        low_euq.setMax(9);

        low_text = (TextView)findViewById(R.id.low_text);
        low_text.setText("LOW: " + low_euq.getProgress());

        low_euq.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            public void onProgressChanged(SeekBar seekBar, int progress_low, boolean fromUser){
                progressChanged_low = progress_low;
                low_text.setText("LOW: " + progress_low);
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                low_text.setText("LOW: " + progressChanged_low);
                sendMessage("1"+Integer.toString(progressChanged_low));
            }
        });


    }

    public void mid( ){

        mid_euq = (SeekBar) findViewById(R.id.mid_bar);
        mid_euq.setProgress(4);
        mid_euq.incrementProgressBy(1);
        mid_euq.setMax(9);

        mid_text = (TextView)findViewById(R.id.mid_text);
        mid_text.setText("MID: " + mid_euq.getProgress());

        mid_euq.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            public void onProgressChanged(SeekBar seekBar, int progress_mid, boolean fromUser){
                progressChanged_mid = progress_mid;
                mid_text.setText("MID: " + progress_mid);
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                mid_text.setText("MID: " + progressChanged_mid);
                sendMessage("2"+Integer.toString(progressChanged_mid));
            }
        });


    }

    public void high( ){

        high_euq = (SeekBar) findViewById(R.id.high_bar);
        high_euq.setProgress(4);
        high_euq.incrementProgressBy(1);
        high_euq.setMax(9);

        high_text = (TextView)findViewById(R.id.high_text);
        high_text.setText("HIGH: " + high_euq.getProgress());

        high_euq.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            public void onProgressChanged(SeekBar seekBar, int progress_high, boolean fromUser){
                progressChanged_high = progress_high;
                high_text.setText("HIGH: " + progress_high);
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                high_text.setText("HIGH: " + progressChanged_high);
                sendMessage("3"+Integer.toString(progressChanged_high));
            }
        });

    }

    //output helper method
    private void searchForDevices ()
    {
        statusUpdate("Searching for devices ...");

        if(mTimer != null) {
            mTimer.cancel();
        }

        scanLeDevice();
        mTimer = new Timer();
        mTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                statusUpdate("Search complete");
                findGroveBLE();
            }
        }, SCAN_PERIOD);
    }

    private void findGroveBLE ()
    {
        if(mDevices == null || mDevices.size() == 0)
        {
            statusUpdate("No BLE devices found");
            return;
        }
        else if(mDevice == null)
        {
            statusUpdate("Unable to find Grove BLE");
            return;
        }
        else
        {
            statusUpdate("Found Grove BLE V1");
            connectDevice();
            statusUpdate("Ready");
        }
    }

    private boolean connectDevice ()
    {
        BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(mDevice.getAddress());
        if (device == null) {
            statusUpdate("Unable to connect");
            return false;
        }
        // directly connect to the device
        statusUpdate("Connecting ...");
        mBluetoothGatt = device.connectGatt(this, false, mGattCallback);
        return true;
    }

    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                //statusUpdate("Connected");
                //statusUpdate("Searching for services");
                mBluetoothGatt.discoverServices();
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                //statusUpdate("Device disconnected");
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                List<BluetoothGattService> gattServices = mBluetoothGatt.getServices();

                for(BluetoothGattService gattService : gattServices) {
                    //statusUpdate("Service discovered: " + gattService.getUuid());
                    if(GROVE_SERVICE.equals(gattService.getUuid().toString()))
                    {
                        mBluetoothGattService = gattService;
                        //statusUpdate("Found communication Service");
                        sendMessage(" ");
                    }
                }
            } else {
                //statusUpdate("onServicesDiscovered received: " + status);
            }
        }
    };

    private void sendMessage (String data)
    {
        if (mBluetoothGattService == null)
            return;

            //statusUpdate("Finding Characteristic...");
            BluetoothGattCharacteristic gattCharacteristic =
                mBluetoothGattService.getCharacteristic(UUID.fromString(CHARACTERISTIC_TX));

        if(gattCharacteristic == null) {
            statusUpdate("Couldn't find TX characteristic: " + CHARACTERISTIC_TX);
            return;
        }

        //statusUpdate("Found TX characteristic: " + CHARACTERISTIC_TX);

        //statusUpdate("Sending message 'Hello Grove BLE'");

        String msg = data;

        byte b = 0x00;
        byte[] temp = msg.getBytes();
        byte[] tx = new byte[temp.length + 1];
        tx[0] = b;

        for(int i = 0; i < temp.length; i++)
            tx[i+1] = temp[i];

        gattCharacteristic.setValue(tx);
        mBluetoothGatt.writeCharacteristic(gattCharacteristic);
    }

    private void scanLeDevice() {
        new Thread() {

            @Override
            public void run() {
                mBluetoothAdapter.startLeScan(mLeScanCallback);

                try {
                    Thread.sleep(SCAN_PERIOD);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                mBluetoothAdapter.stopLeScan(mLeScanCallback);
            }
        }.start();
    }

    private BluetoothAdapter.LeScanCallback mLeScanCallback = new BluetoothAdapter.LeScanCallback() {

        @Override
        public void onLeScan(final BluetoothDevice device, final int rssi, byte[] scanRecord) {
            if (device != null) {
                if (mDevices.indexOf(device) == -1)//to avoid duplicate entries
                {
                    if (DEVICE_NAME.equals(device.getName())) {
                        mDevice = device;//we found our device!

                        mDevices.add(device);
                        statusUpdate("Found device: " + device.getName());
                    }
                }
            }

        }
    };

    //output helper method
    private void statusUpdate (final String msg) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Context context = getApplicationContext();
                CharSequence text = msg;
                int duration = Toast.LENGTH_SHORT;

                Toast toast = Toast.makeText(context, text, duration);
                toast.show();
            }
        });
    }
}
