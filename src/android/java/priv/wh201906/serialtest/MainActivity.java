package priv.wh201906.serialtest;

import org.qtproject.qt5.android.bindings.QtActivity;

import android.util.Log;
import android.view.WindowManager;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.net.Uri;
import android.bluetooth.*;
import android.os.Process;

import java.util.Set;
import java.lang.String;
import java.util.ArrayList;

public class MainActivity extends QtActivity
{
    private static final String LOG_TAG = "MainActivity";
    private Intent startIntent = null;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        // keep screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        // store intent
        startIntent = getIntent();
        // handleIntent() should not be called there.
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);
        handleIntent(intent);
    }

    public void handleStartIntent()
    {
        if (startIntent != null)
        {
            handleIntent(startIntent);
            startIntent = null;
        }
    }

    private void handleIntent(Intent intent)
    {
        // debug output
        String action = intent.getAction();
        String type = intent.getType();
        if (BuildConfig.DEBUG) {
            Set<String> categories = intent.getCategories();
            Log.i(LOG_TAG, "Action:" + (action == null ? "(null)" : action));
            Log.i(LOG_TAG, "Type:" + (type == null ? "(null)" : type));
            if (categories == null)
                Log.i(LOG_TAG, "Categorie:(null)");
            else
            {
                for (String str : categories)
                    Log.i(LOG_TAG, "Categorie:" + str);
            }
        }
        // check action
        if (action == null || !action.equals(Intent.ACTION_SEND) || type == null)
        {
            Log.e(LOG_TAG, "Error:unexpected action/invalid type");
            return;
        }
        String text = intent.getStringExtra(Intent.EXTRA_TEXT);
        Uri fileUri = (Uri) intent.getParcelableExtra(Intent.EXTRA_STREAM);
        // handle text
        if (type.startsWith("text/") && text != null)
        {
            Log.i(LOG_TAG, "handle text");
            if (BuildConfig.DEBUG) {
                Log.i(LOG_TAG, text);
            }
            shareText(text);
        }
        // handle file
        else if(fileUri != null)
        {
            Log.i(LOG_TAG, "handle file");
            if (BuildConfig.DEBUG) {
                Log.i(LOG_TAG, "Uri Permission:" + String.valueOf(checkUriPermission(fileUri, Process.myPid(), Process.myUid(), Intent.FLAG_GRANT_READ_URI_PERMISSION) == PackageManager.PERMISSION_GRANTED));
                Log.i(LOG_TAG, "fileUri:" + fileUri.toString());
            }
            shareFile(fileUri.toString());
        }
        else
        {
            Log.e(LOG_TAG, "Error:unexpected extra");
        }
    }

    public String[] getBondedDevices(boolean isBLE)
    {
        BluetoothAdapter BTAdapter = BluetoothAdapter.getDefaultAdapter();
        Set<BluetoothDevice> bondedDevice = BTAdapter.getBondedDevices();
        ArrayList<String> list = new ArrayList<String>();
        for (BluetoothDevice bt : bondedDevice)
        {
            int deviceType = bt.getType();
            if (!isBLE && (deviceType == BluetoothDevice.DEVICE_TYPE_CLASSIC || deviceType == BluetoothDevice.DEVICE_TYPE_DUAL))
                list.add(bt.getAddress() + " " + bt.getName());
            else if (isBLE && (deviceType == BluetoothDevice.DEVICE_TYPE_LE || deviceType == BluetoothDevice.DEVICE_TYPE_DUAL))
                list.add(bt.getAddress() + " " + bt.getName());
        }
        String[] result = (String[]) list.toArray(new String[list.size()]);
        return result;
    }

    private static native void shareText(String text);
    private static native void shareFile(String text);
}
