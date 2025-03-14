package com.example.myapplication;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.UnknownHostException;
import java.util.Map;
import java.util.Set;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

public class huaweiIOT {

    // 华为账号相关配置
    private static final String HUAWEINAME = "";  // 华为账号名
    private static final String IAMINAME = "";      // IAM账户名
    private static final String IAMPASSWORD = "";    // IAM账户密码
    // IoT相关配置
    private final String project_id = "";  // 项目ID
    private final String device_id = "";   // 设备ID
    private final String service_id = "";  // 服务ID
    private final String command_name = ""; // 命令名称，根据需要修改，我这里没有写控制命令，所以留着备用，暂时没用

    // IoT 接入终端节点
    private final String endpoint = "";

    private String token = ""; // 全局 Token，不用动，自动获取会调用填充

    public huaweiIOT() throws Exception {
        token = gettoken();
    }

    /**
     * 获取设备属性（shadow）或状态（status）
     */
    public String getAtt(String att,String mode) throws Exception{
        String strurl="";
        if(mode.equals("shadow")) strurl="https://%s/v5/iot/%s/devices/%s/shadow";
        if(mode.equals("status"))  strurl="https://%s/v5/iot/%s/devices/%s";
        strurl = String.format(strurl, endpoint,project_id,device_id);
        URL url = new URL(strurl);
        HttpURLConnection urlCon = (HttpURLConnection)url.openConnection();
        urlCon.addRequestProperty("Content-Type", "application/json");
        urlCon.addRequestProperty("X-Auth-Token",token);
        urlCon.connect();
        InputStreamReader is = new InputStreamReader(urlCon.getInputStream());
        BufferedReader bufferedReader = new BufferedReader(is);
        StringBuffer strBuffer = new StringBuffer();
        String line = null;
        while ((line = bufferedReader.readLine()) != null) {
            strBuffer.append(line);
        }
        is.close();
        urlCon.disconnect();
        String result = strBuffer.toString();
        System.out.println(result);
        if(mode.equals("shadow"))
        {
            ObjectMapper objectMapper = new ObjectMapper();
            JsonNode jsonNode = objectMapper.readValue(result, JsonNode.class);
            JsonNode tempNode = jsonNode.get("shadow").get(0).get("reported").get("properties").get(att);
            String attvaluestr = tempNode.asText();
            System.out.println(att+"=" + attvaluestr);
            return attvaluestr;
        }
        if(mode.equals("status"))
        {
            ObjectMapper objectMapper = new ObjectMapper();
            JsonNode jsonNode = objectMapper.readValue(result, JsonNode.class);
            JsonNode statusNode = jsonNode.get("status");
            String statusstr = statusNode.asText();
            System.out.println("status = " + statusstr);
            return statusstr;
        }
        return "error";
    }

    /**
     * 下发命令到设备
     */
    public String setCom(String com, String value) throws Exception {
        String strurl = String.format("https://%s/v5/iot/%s/devices/%s/commands", endpoint, project_id, device_id);
        URL url = new URL(strurl);
        HttpURLConnection urlCon = (HttpURLConnection) url.openConnection();
        urlCon.setRequestProperty("Content-Type", "application/json");
        urlCon.setRequestProperty("X-Auth-Token", token);
        urlCon.setRequestMethod("POST");
        urlCon.setDoOutput(true);
        urlCon.setUseCaches(false);
        urlCon.setInstanceFollowRedirects(true);
        urlCon.connect();

        String body = String.format("{\"paras\":{\"%s\":%s},\"service_id\":\"%s\",\"command_name\":\"%s\"}",
                com, value, service_id, command_name);

        try (BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(urlCon.getOutputStream(), "UTF-8"))) {
            writer.write(body);
            writer.flush();
        }

        StringBuilder strBuffer = new StringBuilder();
        try (BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(urlCon.getInputStream(), "UTF-8"))) {
            String line;
            while ((line = bufferedReader.readLine()) != null) {
                strBuffer.append(line);
            }
        }
        urlCon.disconnect();
        String result = strBuffer.toString();
        System.out.println("setCom Response: " + result);
        return result;
    }

    /**
     * 通过华为云 IAM 接口获取 Token
     */
    public static String gettoken() throws Exception {
        String tokenstr = "{"
                + "\"auth\": {"
                + "\"identity\": {"
                + "\"methods\": [\"password\"],"
                + "\"password\": {"
                + "\"user\": {"
                + "\"domain\": {\"name\": \"" + HUAWEINAME + "\"},"
                + "\"name\": \"" + IAMINAME + "\","
                + "\"password\": \"" + IAMPASSWORD + "\"}}},"
                + "\"scope\": {\"project\": {\"name\": \"cn-north-4\"}}}}";

        String strurl = "https://iam.cn-north-4.myhuaweicloud.com/v3/auth/tokens?nocatalog=false";
        URL url = new URL(strurl);
        HttpURLConnection urlCon = (HttpURLConnection) url.openConnection();
        urlCon.setRequestProperty("Content-Type", "application/json;charset=utf8");
        urlCon.setDoOutput(true);
        urlCon.setRequestMethod("POST");
        urlCon.setUseCaches(false);
        urlCon.setInstanceFollowRedirects(true);
        urlCon.connect();

        try (BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(urlCon.getOutputStream(), "UTF-8"))) {
            writer.write(tokenstr);
            writer.flush();
        }

        // 获取返回的 Token
        String token = urlCon.getHeaderField("X-Subject-Token");
        if (token == null) {
            throw new RuntimeException("获取 Token 失败");
        }

        System.out.println("Token 获取成功：" + token);
        return token;
    }


    // 示例 main 方法调用
    public static void main(String[] args) {
        try {
            huaweiIOT iot = new huaweiIOT();
            // 示例：获取 shadow 模式下某属性值
            iot.getAtt("Temp", "shadow");
            // 示例：获取设备状态
            iot.getAtt("", "status");
            // 示例：下发命令，参数名称 "switch" 值为 true
            iot.setCom("switch", "true");
        } catch (UnknownHostException une) {
            System.err.println("网络异常：无法解析域名，请检查设备网络配置。");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
