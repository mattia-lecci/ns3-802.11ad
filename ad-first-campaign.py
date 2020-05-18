def main():

    import sem
    import math
    import numpy as np
    import os
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    from collections import OrderedDict 

    # Define campaign parameters
    ############################

    script = "evaluate_scheduler_qd_dense_scenario"
    cmp_name = "basic-vs-cbaponly-v2"        # define a campaign name in order to create different corresponding to a specific configuration
    ns_path = os.path.join(os.path.dirname(os.path.realpath(__file__)))
    campaign_dir = "./campaigns/"+cmp_name
    sem.parallelrunner.MAX_PARALLEL_PROCESSES = 32
    print ("ns3-path=", ns_path)
    print ("campaign-dir=", campaign_dir)

    # Create campaign
    #################

    campaign = sem.CampaignManager.new(ns_path, script, campaign_dir,
        runner_type="ParallelRunner",
        check_repo=False,
        skip_config=True,
        optimized=False,
        overwrite=True)

    print (campaign)

    # Parameter space
    #################

    nruns = 10
    dataRate = "100Mbps"
    mcs = "DMG_MCS4"
    simulationTime = 10.0
    logComponentsStr = ":"
    mpduAggregationList = [0, 262143]
    interAllocationList = [10, 1500]
    schedulerList = ["ns3::CbapOnlyDmgWifiScheduler", "ns3::BasicDmgWifiScheduler"]

    param_combinations = OrderedDict ({
        "simulationTime": simulationTime,
        "scheduler": schedulerList,
        "logComponentsStr": logComponentsStr,
        "mpduAggregation": mpduAggregationList,
        "interAllocation": interAllocationList,
        "dataRate": dataRate,
        "phyMode": mcs,
        "RngRun": list(range(nruns))
    })

    # Run simulations
    #################
    print("Run simulations with param_combination " + str(param_combinations))
    campaign.run_missing_simulations (param_combinations)

    print("Simulations done.")

if __name__ == '__main__':
    main()
