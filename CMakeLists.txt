cmake_minimum_required(VERSION 3.22)

project(CraftPlanner VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Sql)

find_package(Boost REQUIRED)

qt_standard_project_setup()

qt_add_executable(${PROJECT_NAME}
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/Plan.h src/Plan.cpp
    src/Step.h src/Step.cpp
    src/PlanModel.h src/PlanModel.cpp
    src/PlanItem.h src/PlanItem.cpp
    src/StepWidget.h src/StepWidget.cpp
    src/PlanWidget.h src/PlanWidget.cpp
    src/ExchangeItemData.h
    src/TradeItemData.h
    src/StepItem.h src/StepItem.cpp
    src/StepItemModel.h src/StepItemModel.cpp
    src/CustomItemData.h
    src/StepItemView.h src/StepItemView.cpp
    src/Settings.h src/Settings.cpp
    src/TradeRequestManager.h src/TradeRequestManager.cpp
    src/WebPage.h src/WebPage.cpp
    src/TradeRequestKey.h src/TradeRequestKey.cpp
    src/TradeRequestCache.h src/TradeRequestCache.cpp
    src/TradeRequestData.h src/TradeRequestData.cpp
    src/ExchangeRequestCache.h src/ExchangeRequestCache.cpp
    src/ExchangeData.h src/ExchangeData.cpp
    src/CurrencyCost.h
    src/Game.h
    src/Database.h src/Database.cpp
    src/InitializationDialog.h src/InitializationDialog.cpp
    src/ExchangeRequestManager.h src/ExchangeRequestManager.cpp
    src/ExchangeCostData.h src/ExchangeCostData.cpp
    src/CurrencyCost.cpp
    src/HashFunctions.h
    src/StepItemDelegate.h src/StepItemDelegate.cpp
    src/TradeCostData.h src/TradeCostData.cpp
    src/RequestEditDialog.h src/RequestEditDialog.cpp
    src/PlanTreeView.h src/PlanTreeView.cpp
    src/Game.cpp
    src/ItemTime.h
    src/UpdateCostDialog.h src/UpdateCostDialog.cpp
    src/CostWidget.h src/CostWidget.cpp
    src/ItemCost.h src/ItemCost.cpp
    src/StepItemData.h
    src/StepItemsWidget.h src/StepItemsWidget.cpp
    src/ImportException.h
    src/ImportOverwriteModel.h src/ImportOverwriteModel.cpp
    src/ImportOverwriteDialog.h src/ImportOverwriteDialog.cpp
    src/SettingsDialog.h src/SettingsDialog.cpp
    src/DescriptionEdit.h src/DescriptionEdit.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::WebEngineWidgets Qt6::Sql Boost::headers)

target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/additional.sqlite3
            ${CMAKE_CURRENT_BINARY_DIR}/additional.sqlite3
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    OUTPUT_NAME "PoE Craft Planner"
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_SOURCE_DIR}/install)
endif()

if(WIN32)
     set_property(
         TARGET ${PROJECT_NAME}
         APPEND PROPERTY
             SOURCES $<TARGET_PROPERTY:${PROJECT_NAME},OUTPUT_NAME>.exe.manifest)
 endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION "."
    RUNTIME DESTINATION "."
)
install(FILES additional.sqlite3 LICENSE
    DESTINATION "."
)

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS --libdir "." --no-compiler-runtime
)

install(SCRIPT ${deploy_script})
