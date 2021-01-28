clear
close all
clc

% need 8 colors
colors = [27,158,119;...
217,95,2;...
117,112,179;...
231,41,138;...
102,166,30;...
230,171,2;...
166,118,29;...
102,102,102] / 255;
set(groot, 'DefaultAxesColorOrder', colors)

%% Input
campaign = ".";

pkts = readtable(fullfile(campaign, "packetsTrace.csv"));
pkts.TxTimestamp_s = pkts.TxTimestamp_ns / 1e9; % TxTimestamp_s is actually in [ns]
pkts.RxTimestamp_s = pkts.RxTimestamp_ns / 1e9; % RxTimestamp_s is actually in [ns]
% Add delay info
pkts.Delay_s = pkts.RxTimestamp_s - pkts.TxTimestamp_s;

app = readtable(fullfile(campaign, "appTrace.csv"));
app.Timestamp_s = app.Timestamp_ns / 1e9; % Timestamp_s is actually in [ns]

sps = readtable(fullfile(campaign, "spTrace.csv"));
sps.Timestamp_s = sps.Timestamp_ns / 1e9; % Timestamp_s is actually in [ns]

%% Global params
nodeId = 1; % reference STA
sta1Mask = pkts.SrcNodeId == nodeId;
numStas = 4;

%% Compute DTI structure
dtiStructure = struct();
srcNodeIds = unique(sps.SrcNodeId);
for i = 1:length(srcNodeIds)
    id = srcNodeIds(i);
    dtiStructure(i).id = id;
    dtiStructure(i).start = sps.Timestamp_s(sps.SrcNodeId == id & sps.isStart == 1);
    dtiStructure(i).end = sps.Timestamp_s(sps.SrcNodeId == id & sps.isStart == 0);
    
    if length(dtiStructure(i).start) > length(dtiStructure(i).end)
        % Proabably simulation end truncated last SP end
        dtiStructure(i).end(end+1) = dtiStructure(i).start(end); % end last SP immediately
    end
    
    if id == 0
        biStart = [0; dtiStructure(i).end];
    end
end

biDuration_s = biStart(2) - biStart(1);

%%

%height = biDuration_s * 1e3;

figure
%plotDti(dtiStructure, height)
set(gca,'ColorOrderIndex', 1)

node = 1;
mask_1 = sps.SrcNodeId == node;
mask_2 = pkts.SrcNodeId == node;
mask_app = app.SrcNodeId == node;

sps_temp = sps(mask_1,:);
pkts_temp = pkts(mask_2,:);
app_temp = app(mask_app,:);

for i=1:2:size(sps_temp,1)
    sp_start = sps_temp(i,:).Timestamp_ns;
    sp_end = sps_temp(i+1,:).Timestamp_ns;
    mask_3 = (pkts_temp.RxTimestamp_ns >= sp_start) & (pkts_temp.RxTimestamp_ns <= sp_end);
    mask_app = (app_temp.Timestamp_ns >= (sp_start-1e6)) & (app_temp.Timestamp_ns <= (sp_end+1e6));
    
    rx_pkts = pkts_temp(mask_3,:);
    time_interval = rx_pkts.RxTimestamp_ns(end) - rx_pkts.TxTimestamp_ns(1);
    
    num_tx_pkts = size(app_temp(mask_app,:),1);
    num_pkts = size(rx_pkts,1);
    thr = (num_pkts * 1448 * 8) / (time_interval/1e9)/ 1e6; % calculate thr in Mbps
    fprintf('SP start %.2fs, SP duration %.2fms, pkt time %.2fms, THR %.2fMbps, %d pkts tx, %d pkts rx \n', sp_start/1e9, (sp_end-sp_start)/1e6, time_interval/1e6, thr, num_tx_pkts, num_pkts);
end

%% UTILS
function plotSp(spStruct, height, alpha)
assert(length(spStruct.start) == length(spStruct.end))
nSps = length(spStruct.start);

x = zeros(1, nSps*4);
x(1:4:end) = spStruct.start;
x(2:4:end) = spStruct.start;
x(3:4:end) = spStruct.end;
x(4:4:end) = spStruct.end;

y = repmat([0,1,1,0] * height, 1, nSps);
area(x, y, 'FaceAlpha', alpha, 'DisplayName', sprintf('SrcNodeId %d', spStruct.id))
end


function plotDti(dtiStructure, height)
alpha = 0.5;

for i = 1:length(dtiStructure)
    if dtiStructure(i).id > 0 && dtiStructure(i).id < 255
        set(gca,'ColorOrderIndex', dtiStructure(i).id)
        plotSp(dtiStructure(i), height, alpha); hold on
    end
end

end