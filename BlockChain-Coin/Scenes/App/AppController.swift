//  
//  AppVC.swift
//  BlockChain-Coin
//
//  Created by Maxime Bornemann on 12/03/2018.
//  Copyright © 2018 BLOC.MONEY. All rights reserved.
//

import UIKit
import SwiftyBeaver

protocol AppDisplayLogic: class {
}

class AppController: AppDisplayLogic {
    
    let window: UIWindow
    let router: AppRoutingLogic
    let interactor: AppBusinessLogic
    
    static var current: AppController = {
        return (UIApplication.shared.delegate as! AppDelegate).app
    }()
    
    static var environment: Environment {
        let env = ProcessInfo.processInfo.environment
        
        if let isTesting = env["UITests"], isTesting == "true" {
            return .mock
        }
        
        return .production
    }

    // MARK: - View lifecycle
    
    init() {
        let window = UIWindow(frame: UIScreen.main.bounds)
        
        let interactor = AppInteractor()
        let presenter = AppPresenter()
        let router = AppRouter()
        
        self.window = window
        self.router = router
        self.interactor = interactor
        
        interactor.presenter = presenter
        presenter.viewController = self
        router.window = window
        
        postInit()
    }
    
    init(window: UIWindow = UIWindow(frame: UIScreen.main.bounds),
         router: AppRoutingLogic,
         interactor: AppBusinessLogic) {
        self.window = window
        self.router = router
        self.interactor = interactor
        
        postInit()
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    fileprivate func postInit() {
        configure()
        
        router.showApp()
        router.showHome()
    }

    // MARK: - Configuration
    
    func configure() {
        configureUserDefaults()
        configureEnvironment()
        configureLogging()
        
        log.info("Current environment: \(Environment.current)")
    }

    fileprivate func configureUserDefaults() {
        UserDefaults.standard.register(defaults: [:])
    }

    fileprivate func configureLogging() {
        let console = ConsoleDestination()
        console.minLevel = SwiftyBeaver.Level(rawValue: Environment.current.logLevel.rawValue) ?? .error
        
        let file = FileDestination()
        file.minLevel = .info
        
        log.info("Logging to \(file.logFileURL)")
        
        log.addDestination(file)
        log.addDestination(console)
    }

    // If this is the first time we launch the app, and the environment has
    // not been set in the user defaults, we get the default environment from
    // the Info.plist
    fileprivate func configureEnvironment() {
        let isFirstLaunch = (Environment.settingsEnvironment == nil)
        
        if isFirstLaunch {
            UserDefaults.standard.environment = Environment.bundleEnvironment
        }
    }
    
    // MARK: - Helpers
    
    func dismissAllModals(_ completion: (() -> ())?) {
        if let presentedController = window.rootViewController?.presentedViewController {
            presentedController.dismiss(animated: true, completion: {
                if let completion = completion {
                    completion()
                }
            })
        }
        else {
            if let completion = completion {
                completion()
            }
        }
    }
}
